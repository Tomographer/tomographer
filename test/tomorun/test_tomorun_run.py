
from __future__ import print_function

import sys
import re
import os.path
import subprocess
import argparse

import logging
logging.basicConfig(level=logging.DEBUG)
logger = logging.getLogger(__name__)

import numpy as np


def check_qeb_args(x):
    if isinstance(x,tuple):
        return x
    m = re.match(r"^\((?P<f0>[0-9.e+-]+),(?P<Delta>[0-9.e+-]+),(?P<gamma>[0-9.e+-]+)\),(?P<Error>\d+)%$", x)
    if m is None:
        raise ValueError("check-qeb: expected argument in format \"(<f0>,<Delta>,<gamma>),<Error>%\"")
    return (float(m.group('f0')),float(m.group('Delta')),float(m.group('gamma')),float(m.group('Error'))/100.0)

def ftox_pair(ftoxstr):
    if isinstance(ftoxstr,tuple):
        return ftoxstr
    m = re.match(r"^\((?P<h>[0-9.e+-]+),(?P<s>[+-]?1)\)$", ftoxstr)
    if m is None:
        raise ValueError("ftox: expected \"(h,s)\" pair")
    return (float(m.group('h')), int(m.group('s')))

def percent_within_range_type(x):
    if isinstance(x, tuple):
        return x
    m = re.match(r"^(?P<percent>[0-9.e+-]+)%\s*\((?P<min>[0-9.e+-]+),\s*(?P<max>[0-9.e+-]+)\)$", x)
    if m is None:
        raise ValueError("percent_within_range: expected \"<percent>% (<min>,<max>)\" format")
    return (float(m.group('percent')), float(m.group('min')), float(m.group('max')))

def run_main():
    parser = argparse.ArgumentParser("test_tomorun_run")
    parser.add_argument("tomorun_argv", nargs='+')
    parser.add_argument("--setpath", action='store')
    parser.add_argument("--expect-exit-code", action='store', type=int, default=0)
    parser.add_argument("--no-further-checks", action='store_true', default=False)
    parser.add_argument("--check-histogram-file", action='store', default=None)
    parser.add_argument("--check-qeb", action='store', type=check_qeb_args)
    parser.add_argument("--use-qeb-hint", action='store_true', default=False)
    parser.add_argument("--ftox", action='store', type=ftox_pair, default=(0,1,))
    parser.add_argument("--check-percent-within-range", action='append', type=percent_within_range_type)

    args = parser.parse_args()

    # setup sys.path, if needed
    if hasattr(args,'setpath') and args.setpath is not None:
        for p in reversed(args.setpath.split(':')):
            if p:
                sys.path.insert(0, p)
        logger.debug("Set sys.path = {!r}".format(sys.path))

    # first, launch the tomorun process
    try:
        logger.debug("Launching tomorun process with argv={!r}".format(args.tomorun_argv))
        subprocess.check_call(args.tomorun_argv)
    except subprocess.CalledProcessError as e:
        if e.returncode != args.expect_exit_code:
            raise
        logger.debug("Got the expected return code %d"%(args.expect_exit_code))

    did_check = False

    # load the output histogram
    if args.check_histogram_file:

        if hasattr(args,'check_qeb') and args.check_qeb is not None:

            # check the quantum error bars
            do_check_qeb(args.check_histogram_file, args.check_qeb, ftox=args.ftox, use_qeb_hint=args.use_qeb_hint)
            did_check = True

        if hasattr(args,'check_percent_within_range') and args.check_percent_within_range is not None:

            # check that a certain percentatge of the total weight falls within the given
            # range (can be a list of ranges), if the option is specified multiple times
            for check_rng in args.check_percent_within_range:
                do_check_percent_within_range(args.check_histogram_file, check_rng)
            did_check = True

    if args.no_further_checks:
        return

    if not did_check:
        raise RuntimeError("Didn't check anything.")
        

def assert_approx_eq_rel(x, y, err):
    print("Testing {!r} =? {!r}  (to {!r} rel. precision) ...".format(x,y,err), end='')
    refom = np.maximum(np.absolute(x),np.absolute(y))
    assert np.absolute(x-y) < err*refom
    print(" OK")


def do_check_percent_within_range(hfile, percent_within_range):
    # only import tomographer here, after possibly having set sys.path
    import tomographer as t
    import tomographer.querrorbars as tq
    logger.debug("Loaded tomographer from {} and tomographer.querrorbars from {}".format(t.__path__, tq.__file__))
    
    h = tq.load_tomorun_csv_histogram_file(hfile)
    
    percent, rngmin, rngmax = percent_within_range
    
    if h.params.isWithinBounds(rngmin):
        idxmin = h.params.binIndex(rngmin)
    else:
        idxmin = 0
    if h.params.isWithinBounds(rngmax):
        idxmax = h.params.binIndex(rngmax)
    else:
        idxmax = h.numBins()-1

    w = np.sum(h.bins[idxmin:idxmax])

    logger.debug("Total weight within [{:.4g},{:.4g}] is {:.3g}%; required is {:.3g}%"
                 .format(rngmin,rngmax, w*100.0, percent))

    print("Testing {!r} > {!r} ... ".format(w, percent/100.0), end='')
    assert w > percent/100.0
    print(" OK")


def do_check_qeb(hfile, refqeb, ftox, use_qeb_hint):
    # only import tomographer here, after possibly having set sys.path
    import tomographer as t
    import tomographer.querrorbars as tq
    logger.debug("Loaded tomographer from {} and tomographer.querrorbars from {}".format(t.__path__, tq.__file__))
    
    h = tq.load_tomorun_csv_histogram_file(hfile)

    kwargs = {}
    if use_qeb_hint:
        ftoxfn = lambda f, s=ftox[1], h=ftox[0]: s*(f - h)
        kwargs['p0'] = tq.reskew_logmu_curve(*tq.qu_error_bars_to_deskewed_c(ftoxfn, *(refqeb[:3]),
                                                                             y0=np.amax(h.bins)))
        logging.debug("Using QuErrBar hints {!r} -> {!r}".format(refqeb[:3], kwargs['p0']))
    
    a = tq.HistogramAnalysis(h, ftox=ftox, **kwargs)

    # make sure the fit was ok
    redchi2 = a.fitRedChi2()
    logger.debug("redchi2 = {:.4g}".format(redchi2))
    assert_approx_eq_rel(redchi2, 1.0, 0.5) # 0.5--1.5

    a.printQuantumErrorBars(print_func=logger.debug)
    qeb = a.quantumErrorBars()

    assert_approx_eq_rel(qeb.f0, refqeb[0], refqeb[3])
    assert_approx_eq_rel(qeb.Delta, refqeb[1], refqeb[3])
    assert_approx_eq_rel(qeb.gamma, refqeb[2], refqeb[3])




if __name__ == '__main__':
    run_main()
