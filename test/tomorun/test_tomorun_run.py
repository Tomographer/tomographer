
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

    # load the output histogram
    if args.check_histogram_file:

        if hasattr(args,'check_qeb') and args.check_qeb is not None:

            # check the quantum error bars
            do_check_qeb(args.check_histogram_file, args.check_qeb, ftox=args.ftox, use_qeb_hint=args.use_qeb_hint)
            return

    if args.no_further_checks:
        return

    raise RuntimeError("Nothing to check.")
        

def assert_approx_eq_rel(x, y, err):
    print("Testing {!r} =? {!r}  (to {!r} rel. precision) ...".format(x,y,err), end='')
    refom = np.maximum(np.absolute(x),np.absolute(y))
    assert np.absolute(x-y) < err*refom
    print(" OK")

def do_check_qeb(hfile, refqeb, ftox, use_qeb_hint):
    # only import tomographer here, after possibly having set sys.path
    import tomographer as t
    import tomographer.querrorbars as tq
    logger.debug("Loaded tomographer from {} and tomographer.querrorbars from {}".format(t.__path__, tq.__file__))
    
    h = tq.load_tomorun_csv_histogram_file(hfile)

    kwargs = {}
    if use_qeb_hint:
        kwargs['p0'] = tq.reskew_logmu_curve(m, *qu_error_bars_to_deskewed_c(ftox, *(refqeb[:3]),
                                                                             y0=np.maximum(h.bins)))
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
