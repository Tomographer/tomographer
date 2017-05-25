
import re
import argparse
import logging
logger = logging.getLogger(__name__)


import numpy as np

import tomographer
import tomographer.querrorbars


def ftox_pair(ftoxstr):
    if isinstance(ftoxstr,tuple):
        return ftoxstr
    m = re.match(r"^\((?P<h>[0-9.e+-]+),(?P<s>[+-]?1)\)$", ftoxstr)
    if m is None:
        raise ValueError("ftox: expected \"(h,s)\" pair")
    return (float(m.group('h')), int(m.group('s')))


def analyze_histogram(args):

    final_histogram = tomographer.querrorbars.load_tomorun_csv_histogram_file(args.histogram)

    logger.debug("\n"+final_histogram.prettyPrint())

    assert np.absolute(np.absolute(args.ftox[1]) - 1) < 1e-6 # args.ftox[1] is +1 or -1
    logger.debug("Using f = {} {} x".format(args.ftox[0], '+' if args.ftox[1] > 0 else '-'))

    a = tomographer.querrorbars.HistogramAnalysis(final_histogram, ftox=args.ftox)

    a.printFitParameters(print_func=logger.debug)
    a.printQuantumErrorBars(print_func=logger.info)

    if args.show_plot:
        a.plot(curve_npts=args.curve_npts)
    if args.show_plot_log:
        a.plot(curve_npts=args.curve_npts, log_scale=True)

def run_main():
    logging.basicConfig(level=logging.INFO, format='[%(levelname)s] %(message)s')

    parser = argparse.ArgumentParser("analyze_tomorun_histogram_simple")
    parser.add_argument("histogram",
                        help='Histogram file to load')
    parser.add_argument("--ftox", action='store', type=ftox_pair, default=(0,1),
                        help="f-to-x conversion function, specified as pair (h,s).  (See "
                        "tomographer.querrorbars.HistogramAnalysis for more info)")
    parser.add_argument("--curve-npts", action='store', type=int, default=500,
                        help="Number of points to use to display the smooth fit curve")

    parser.add_argument("--show-plot", dest='show_plot', action='store_true', default=True,
                        help="Show plot of the distribution of the figure of merit.")
    parser.add_argument("--no-show-plot", dest='show_plot', action='store_false',
                        help="Don't show plot of the distribution of the figure of merit.")
    parser.add_argument("--show-plot-log", dest='show_plot_log', action='store_true', default=True,
                        help="Show plot of the distribution of the figure of merit on log scale.")
    parser.add_argument("--no-show-plot-log", dest='show_plot_log', action='store_false',
                        help="Don't show plot of the distribution of the figure of merit on log scale.")
    parser.add_argument("--verbose", action='store_true', default=False,
                        help="print out more verbose info")

    args = parser.parse_args()

    if args.verbose:
        logging.getLogger().setLevel(logging.DEBUG)
        tomographer.cxxlogger.level = logging.DEBUG
        logger.debug("Verbose logging enabled")

    analyze_histogram(args)


if __name__ == '__main__':
    run_main()
