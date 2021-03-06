#!/usr/bin/env python

from __future__ import print_function

import re
import numpy as np
import numpy.testing as npt

import logging
logging.basicConfig(level=logging.NOTSET) # test LONGDEBUG logging level

import unittest

# import the module
import tomographer


class BasicStuff(unittest.TestCase):
    def test_version(self):
        # test for __version__
        print("Tomographer version: "+tomographer.__version__)
        # parse __version__
        m = re.match(r'^v(?P<maj>\d+)\.(?P<min>\d+)(?P<suffix>[a-zA-Z][a-zA-Z0-9]*)?'
                     r'(-(?P<gitsuffix>\d+-?g?[a-fA-F0-9]*))?$',
                     tomographer.__version__)
        self.assertIsNotNone(m)
        print("Verision MAJ=", m.group('maj'), " MIN=", m.group('min'), " SUFFIX=", m.group('suffix'),
              " GITSUFFIX=", m.group('gitsuffix'))
        self.assertTrue(m.group('maj') == str(tomographer.version.version_info.major))
        self.assertTrue(m.group('min') == str(tomographer.version.version_info.minor))

        print("CFLAGS = "+repr(tomographer.version.compile_info['cflags']))
        self.assertTrue('cflags' in tomographer.version.compile_info)
        self.assertTrue('eigen' in tomographer.version.compile_info)
        self.assertTrue('boost' in tomographer.version.compile_info)

    def test_cxxlogger(self):
        # test that the cxxlogger object exists, and check we can set a log level.
        tomographer.cxxlogger.level = logging.INFO

    def test_exception(self):
        # test that the C++ code is able to raise an exception
        with self.assertRaises(ValueError):
            # wrong array shape & dimensions -- should cause an exception
            tomographer.Histogram().load(np.array([ [1, 2], [3, 4] ]))




class Histograms(unittest.TestCase):
    def test_HistogramParams(self):
        # constructors
        params = tomographer.HistogramParams(2.0, 3.0, 5)
        self.assertAlmostEqual(params.min, 2.0)
        self.assertAlmostEqual(params.max, 3.0)
        self.assertEqual(params.num_bins, 5)
        npt.assert_array_almost_equal(params.values_lower, np.array([2.0, 2.2, 2.4, 2.6, 2.8]))
        npt.assert_array_almost_equal(params.values_upper, np.array([2.2, 2.4, 2.6, 2.8, 3.0]))
        npt.assert_array_almost_equal(params.values_center, np.array([2.1, 2.3, 2.5, 2.7, 2.9]))
        # default constructor
        paramsdflt = tomographer.HistogramParams()
        self.assertTrue(paramsdflt.min < paramsdflt.max and paramsdflt.num_bins > 0)
        # w/ keyword arguments
        params = tomographer.HistogramParams(min=2.0, max=3.0, num_bins=5)
        self.assertAlmostEqual(params.min, 2.0)
        self.assertAlmostEqual(params.max, 3.0)
        self.assertEqual(params.num_bins, 5)

        # binCenterValue()
        self.assertAlmostEqual(params.binCenterValue(0), 2.1)
        self.assertAlmostEqual(params.binCenterValue(4), 2.9)
        with self.assertRaises(tomographer.TomographerCxxError):
            x = params.binCenterValue(999)
        with self.assertRaises(tomographer.TomographerCxxError):
            x = params.binCenterValue(5)
        # binLowerValue()
        self.assertAlmostEqual(params.binLowerValue(0), 2.0)
        self.assertAlmostEqual(params.binLowerValue(4), 2.8)
        with self.assertRaises(tomographer.TomographerCxxError):
            x = params.binLowerValue(999)
        with self.assertRaises(tomographer.TomographerCxxError):
            x = params.binLowerValue(5)
        # binUpperValue()
        self.assertAlmostEqual(params.binUpperValue(0), 2.2)
        self.assertAlmostEqual(params.binUpperValue(4), 3.0)
        with self.assertRaises(tomographer.TomographerCxxError):
            x = params.binUpperValue(999)
        with self.assertRaises(tomographer.TomographerCxxError):
            x = params.binUpperValue(5)
        #
        self.assertAlmostEqual(params.binResolution(), 0.2)
        self.assertTrue(params.isWithinBounds(2.0))
        self.assertTrue(params.isWithinBounds(2.2))
        self.assertFalse(params.isWithinBounds(3.0001))
        self.assertFalse(params.isWithinBounds(1.99))

        # repr()
        self.assertEqual(repr(params), 'HistogramParams(min=2,max=3,num_bins=5)')
        
    def test_Histogram(self):
        self.do_test_hist(tomographer.Histogram, int, False)

    # def test_HistogramReal(self):
    #     self.do_test_hist(tomographer.HistogramReal, float, False)

    def test_HistogramWithErrorBars(self):
        self.do_test_hist(tomographer.HistogramWithErrorBars, float, True)

    def do_test_hist(self, HCl, cnttype, has_error_bars):
        print("do_test_hist()")
        # constructor & params member
        self.assertAlmostEqual(HCl(2.0, 3.0, 5).params.min, 2.0)
        self.assertAlmostEqual(HCl(2.0, 3.0, 5).params.max, 3.0)
        self.assertEqual(HCl(2.0, 3.0, 5).params.num_bins, 5)
        self.assertAlmostEqual(HCl(tomographer.HistogramParams(2.0, 3.0, 5)).params.min, 2.0)
        self.assertAlmostEqual(HCl(tomographer.HistogramParams(2.0, 3.0, 5)).params.max, 3.0)
        self.assertEqual(HCl(tomographer.HistogramParams(2.0, 3.0, 5)).params.num_bins, 5)
        h = HCl() # has default constructor
        print("Default histogram parameters: ", h.params.min, h.params.max, h.params.num_bins)
        # values_center, values_lower, values_upper
        npt.assert_array_almost_equal(HCl(2.0,3.0,5).values_lower, np.array([2.0, 2.2, 2.4, 2.6, 2.8]))
        npt.assert_array_almost_equal(HCl(2.0,3.0,5).values_upper, np.array([2.2, 2.4, 2.6, 2.8, 3.0]))
        npt.assert_array_almost_equal(HCl(2.0,3.0,5).values_center, np.array([2.1, 2.3, 2.5, 2.7, 2.9]))
        # constructor sets zero bin values & off_chart
        h = HCl()
        npt.assert_array_almost_equal(h.bins, np.zeros(h.numBins()))
        self.assertAlmostEqual(h.off_chart, 0) # almost-equal in case cnttype=float
        if has_error_bars: npt.assert_array_almost_equal(h.delta, np.zeros(h.numBins()))
        h = HCl(2.0, 3.0, 5)
        npt.assert_array_almost_equal(h.bins, np.array([0,0,0,0,0]))
        self.assertAlmostEqual(h.off_chart, 0) # almost-equal in case cnttype=float
        if has_error_bars: npt.assert_array_almost_equal(h.delta, np.zeros(h.numBins()))
        h = HCl(tomographer.HistogramParams(2.0, 3.0, 5))
        npt.assert_array_almost_equal(h.bins, np.array([0,0,0,0,0]))
        self.assertAlmostEqual(h.off_chart, 0) # almost-equal in case cnttype=float
        if has_error_bars: npt.assert_array_almost_equal(h.delta, np.zeros(h.numBins()))

        # has_error_bars
        h = HCl(2.0, 3.0, 5)
        self.assertTrue(h.has_error_bars == has_error_bars)

        # numBins()
        h = HCl(2.0, 3.0, 5)
        self.assertEqual(h.numBins(), 5)

        # bins, off_chart
        h = HCl(2.0, 3.0, 5)
        h.bins = np.array([1, 4, 9, 5, 2])
        npt.assert_array_almost_equal(h.bins, np.array([1, 4, 9, 5, 2]))
        h.off_chart = 156
        self.assertAlmostEqual(h.off_chart, 156) # almost-equal in case cnttype=float

        # delta [if error bars]
        if has_error_bars:
            h = HCl(2.0, 3.0, 5)
            h.delta = np.array([1, 2, 3, 4, 0.25])
            npt.assert_array_almost_equal(h.delta, np.array([1, 2, 3, 4, 0.25]))

        # count()
        h = HCl(2.0, 3.0, 5)
        h.bins = np.array([1, 4, 9, 5, 2])
        h.off_chart = 17
        for i in range(h.numBins()):
            self.assertAlmostEqual(h.bins[i], h.count(i))
        with self.assertRaises(IndexError):
            n = h.count(9999) # out of range

        # errorBar() [if error bars]
        if has_error_bars:
            h = HCl(2.0, 3.0, 5)
            h.delta = np.array([1, 2, 3, 4, 0.25])
            for i in range(h.numBins()):
                self.assertAlmostEqual(h.delta[i], h.errorBar(i))
            with self.assertRaises(IndexError):
                n = h.errorBar(9999) # out of range

        # load()
        def load_values_maybe_error_bars(h, values, errors, off_chart=None):
            if off_chart is None:
                if not has_error_bars:
                    h.load(values)
                else:
                    h.load(values, errors)
            else:
                if not has_error_bars:
                    h.load(values, off_chart)
                else:
                    h.load(values, errors, off_chart)
        # load(x[, e])
        h = HCl(2.0, 3.0, 5)
        h.bins = np.array([1, 4, 9, 5, 2])
        load_values_maybe_error_bars(h, np.array([10, 20, 30, 40, 50]), np.array([1, 2, 3, 4, 5]))
        npt.assert_array_almost_equal(h.bins, np.array([10, 20, 30, 40, 50]))
        if has_error_bars:
            npt.assert_array_almost_equal(h.delta, np.array([1, 2, 3, 4, 5]))
        self.assertAlmostEqual(h.off_chart, 0) # almost-equal in case cnttype=float
        # load(x[, e], off_chart)
        h = HCl(2.0, 3.0, 5)
        h.bins = np.array([1, 4, 9, 5, 2])
        load_values_maybe_error_bars(h, np.array([10, 20, 30, 40, 50]), np.array([1, 2, 3, 4, 5]), 28)
        npt.assert_array_almost_equal(h.bins, np.array([10, 20, 30, 40, 50]))
        if has_error_bars:
            npt.assert_array_almost_equal(h.delta, np.array([1, 2, 3, 4, 5]))
        self.assertAlmostEqual(h.off_chart, 28) # almost-equal in case cnttype=float
        # load(x, e) error if wrong signature
        if has_error_bars:
            with self.assertRaises(Exception): h.load(np.array([10, 20, 30, 40, 50]))
            with self.assertRaises(Exception): h.load(np.array([10, 20, 30, 40, 50]), 10.0)
        else:
            with self.assertRaises(Exception): h.load(np.array([10, 20, 30, 40, 50]), np.array([1, 2, 3, 4, 5]))
            with self.assertRaises(Exception): h.load(np.array([10, 20, 30, 40, 50]), np.array([1, 2, 3, 4, 5]), 10.0)
        
        # add()
        if not has_error_bars:
            h = HCl(2.0, 3.0, 5)
            h.bins = np.array([1, 4, 9, 5, 2])
            h.add(np.array([10, 20, 30, 40, 50]))
            npt.assert_array_almost_equal(h.bins, np.array([11, 24, 39, 45, 52]))
            self.assertAlmostEqual(h.off_chart, 0) # almost-equal in case cnttype=float
        
        # add()
        if not has_error_bars:
            h = HCl(2.0, 3.0, 5)
            h.bins = np.array([1, 4, 9, 5, 2])
            h.off_chart = 4
            h.add(np.array([10, 20, 30, 40, 50]), 28)
            npt.assert_array_almost_equal(h.bins, np.array([11, 24, 39, 45, 52]))
            self.assertAlmostEqual(h.off_chart, 32) # almost-equal in case cnttype=float

        # reset()
        h = HCl(2.0, 3.0, 5)
        load_values_maybe_error_bars(h, np.array([10, 20, 30, 40, 50]), np.array([1, 2, 3, 4, 5]), 28)
        h.reset()
        npt.assert_array_almost_equal(h.bins, np.array([0,0,0,0,0]))
        self.assertAlmostEqual(h.off_chart, 0) # almost-equal in case cnttype=float

        # record()
        if not has_error_bars:
            h = HCl(2.0, 3.0, 5)
            load_values_maybe_error_bars(h, np.array([10, 20, 30, 40, 50]), np.array([1, 2, 3, 4, 5]), 28)
            h.record(2.569)
            self.assertAlmostEqual(h.count(2), 31)
            h.record(2.569, cnttype(2))
            self.assertAlmostEqual(h.count(2), 33)
            h.record(2.981)
            self.assertAlmostEqual(h.count(4), 51)
            h.record(2.981, cnttype(7))
            self.assertAlmostEqual(h.count(4), 58)

        # normalization(), normalized()
        h = HCl(2.0, 3.0, 5)
        load_values_maybe_error_bars(h, np.array([10, 20, 30, 40, 50]), np.array([1, 2, 3, 4, 5]), 28)
        #print("BINS=",repr(h.bins), repr(h.off_chart))
        n = h.normalization()
        #print("NORMALIZATION=",n)
        self.assertAlmostEqual(n, np.sum(np.array([10, 20, 30, 40, 50])) / 5.0 + 28)
        hn = h.normalized()
        npt.assert_array_almost_equal(hn.bins, h.bins / n)
        self.assertAlmostEqual(hn.off_chart, h.off_chart / n)
        if has_error_bars:
            npt.assert_array_almost_equal(hn.delta, h.delta / n)
        self.assertAlmostEqual(hn.normalization(), 1.0)
        # totalCounts(), normalizedCounts()
        c = h.totalCounts()
        print("TOTALCOUNTS=",c)
        self.assertAlmostEqual(c, np.sum(np.array([10, 20, 30, 40, 50])) + 28)
        hc = h.normalizedCounts()
        npt.assert_array_almost_equal(hc.bins, np.true_divide(h.bins, c))
        self.assertAlmostEqual(hc.off_chart, np.true_divide(h.off_chart, c))
        if has_error_bars:
            npt.assert_array_almost_equal(hc.delta, np.true_divide(h.delta, c))
        self.assertAlmostEqual(hc.totalCounts(), 1.0)

        # prettyPrint()
        h = HCl(2.0, 3.0, 5)
        load_values_maybe_error_bars(h, np.array([10, 20, 30, 40, 50]), np.array([1, 2, 3, 4, 5]), 28)
        s = h.prettyPrint()
        print(s)
        s = h.prettyPrint(120)
        for line in s.split('\n'):
            if line:
                self.assertEqual(len(line), 120)

        # see http://pybind11.readthedocs.io/en/master/advanced/classes.html#pickling-support
        try:
            import cPickle as pickle
        except:
            import pickle
        h = HCl(2.0, 3.0, 5)
        load_values_maybe_error_bars(h, np.array([10, 20, 30, 40, 50]), np.array([1, 2, 3, 4, 5]), 28)
        s = pickle.dumps(h, 2)
        print("PICKLE:\n"+str(s))
        h2 = pickle.loads(s)
        self.assertAlmostEqual(h2.params.min, h.params.min)
        self.assertAlmostEqual(h2.params.max, h.params.max)
        self.assertEqual(h2.params.num_bins, h.params.num_bins)
        npt.assert_array_almost_equal(h2.bins, h.bins)
        if has_error_bars: npt.assert_array_almost_equal(h2.delta, h.delta)


    # def test_AveragedSimpleHistogram(self):
    #     self.do_test_hist(tomographer.AveragedSimpleHistogram, float, True)
    #     self.do_test_avghist(tomographer.AveragedSimpleHistogram,
    #                          tomographer.Histogram, int, False)

    # def test_AveragedSimpleRealHistogram(self):
    #     self.do_test_hist(tomographer.AveragedSimpleRealHistogram, float, True)
    #     self.do_test_avghist(tomographer.AveragedSimpleRealHistogram,
    #                          tomographer.HistogramReal, float, False)

    # def test_AveragedErrorBarHistogram(self):
    #     self.do_test_hist(tomographer.AveragedErrorBarHistogram, float, True)
    #     self.do_test_avghist(tomographer.AveragedErrorBarHistogram,
    #                          tomographer.HistogramWithErrorBars, int, True)

    def do_test_avghist(self, AvgHCl, BaseHCl, cnttyp, base_has_error_bars):
        #
        # test that the AvgHCl can average histograms correctly.  Use the same test cases
        # as in the C++ test.
        #
        print("do_test_avghist()")

        param = tomographer.HistogramParams(0.0, 1.0, 4)

        # constructor & histogram-related methods already tested in do_test_hist

        avghist = AvgHCl(param)
        self.assertEqual(avghist.num_histograms, 0)

        if not base_has_error_bars:
            OFF_IDX = 1
            data = [
                (np.array([15, 45, 42, 12]), 36),
                (np.array([17, 43, 40, 18]), 32),
                (np.array([20, 38, 47, 10]), 35),
                (np.array([18, 44, 43, 13]), 32),
            ]
        else:
            OFF_IDX = 2
            data = [
                (np.array([15, 45, 42, 12]), np.array([1, 1, 1, 1]), 36),
                (np.array([17, 43, 40, 18]), np.array([2, 2, 5, 2]), 32),
                (np.array([20, 38, 47, 10]), np.array([1, 2, 13, 4]), 35),
                (np.array([18, 44, 43, 13]), np.array([2, 1, 24, 3]), 32),
            ]

        k = 0
        for dat in data:
            h = BaseHCl(param)
            h.load(*dat)
            avghist.addHistogram(h)
            k += 1
            self.assertEqual(avghist.num_histograms, k)

        avghist.finalize()

        avgdata = np.array([ sum([ float(data[k][0][n]) for k in range(len(data)) ]) / avghist.num_histograms
                             for n in range(4) ])
        if not base_has_error_bars:
            avgerr = np.array([
                np.sqrt(
                    (sum([ float(data[k][0][n])**2 for k in range(len(data)) ])/avghist.num_histograms - (avgdata[n])**2)
                    / (avghist.num_histograms - 1)
                )
                for n in range(4)
            ])
        else:
            avgerr = np.array([ np.sqrt(sum([ float(data[k][1][n])**2 for k in range(len(data)) ]))
                                / avghist.num_histograms
                                for n in range(4) ])
        avgoff = sum([ float(data[k][OFF_IDX]) for k in range(len(data)) ]) / avghist.num_histograms
        npt.assert_array_almost_equal(avghist.bins, avgdata)
        npt.assert_array_almost_equal(avghist.delta, avgerr)
        self.assertAlmostEqual(avghist.off_chart, avgoff)
        self.assertAlmostEqual(np.sum(avghist.bins) + avghist.off_chart, 150)

        # check for reset(param)
        param2 = tomographer.HistogramParams(1.0, 2.0, 20)
        avghist.reset(param2)
        self.assertEqual(avghist.num_histograms, 0)
        npt.assert_array_almost_equal(avghist.bins, np.zeros(20))
        self.assertAlmostEqual(avghist.off_chart, 0)
        self.assertAlmostEqual(avghist.params.min, param2.min)
        self.assertAlmostEqual(avghist.params.max, param2.max)
        self.assertEqual(avghist.params.num_bins, param2.num_bins)

        # dummy -- add a histogram again ...
        h = BaseHCl(param2)
        if not base_has_error_bars:
            h.load(np.array(range(20)))
        else:
            h.load(np.array(range(20)), np.array(range(20))/10.0)
        avghist.addHistogram(h)        

        # ... and check for reset()
        avghist.reset()
        self.assertEqual(avghist.num_histograms, 0)
        npt.assert_array_almost_equal(avghist.bins, np.zeros(20))
        self.assertAlmostEqual(avghist.off_chart, 0)
        # make sure that params have been kept
        self.assertAlmostEqual(avghist.params.min, param2.min)
        self.assertAlmostEqual(avghist.params.max, param2.max)
        self.assertEqual(avghist.params.num_bins, param2.num_bins)
        
        # test pickling
        # see http://pybind11.readthedocs.io/en/master/advanced/classes.html#pickling-support
        try:
            import cPickle as pickle
        except:
            import pickle
        avghist = AvgHCl(param2)
        # add some histograms
        h = BaseHCl(param2)
        if not base_has_error_bars:
            h.load(np.array(range(20)))
        else:
            h.load(np.array(range(20)), np.array(range(20))/10.0)
        avghist.addHistogram(h)
        h = BaseHCl(param2)
        if not base_has_error_bars:
            h.load(np.array(range(20)))
        else:
            h.load(np.array(range(20)), np.array(range(20))/10.0)
        avghist.addHistogram(h)
        # and now do the pickling
        s = pickle.dumps(avghist,2)
        avghist2 = pickle.loads(s)
        self.assertAlmostEqual(avghist2.params.min, avghist.params.min)
        self.assertAlmostEqual(avghist2.params.max, avghist.params.max)
        self.assertEqual(avghist2.params.num_bins, avghist.params.num_bins)
        npt.assert_array_almost_equal(avghist2.bins, avghist.bins)
        npt.assert_array_almost_equal(avghist2.delta, avghist.delta)
        self.assertEqual(avghist2.num_histograms, avghist.num_histograms)




class MHRWStuff(unittest.TestCase):
    def test_MHRWParams(self):
        # default constructor
        mhrw = tomographer.MHRWParams()
        # constructor with parameters
        mhrw = tomographer.MHRWParams({'step_size': 0.01}, 100, 500, 32768)
        # constructor with keyword arguments
        mhrw = tomographer.MHRWParams(step_size=0.01, n_sweep=100, n_therm=500, n_run=32768)

        # make sure params are stored correctly
        self.assertAlmostEqual(mhrw.mhwalker_params["step_size"], 0.01)
        self.assertEqual(mhrw.n_sweep, 100)
        self.assertEqual(mhrw.n_therm, 500)
        self.assertEqual(mhrw.n_run, 32768)

        # attributes should be writable
        mhrw.mhwalker_params = {'step_size': 0.06}
        self.assertAlmostEqual(mhrw.mhwalker_params["step_size"], 0.06)
        mhrw.n_sweep = 200
        self.assertEqual(mhrw.n_sweep, 200)
        mhrw.n_therm = 1024
        self.assertEqual(mhrw.n_therm, 1024)
        mhrw.n_run = 8192
        self.assertEqual(mhrw.n_run, 8192)




# normally, this is not needed as we are being run via pyruntest.py, but it might be
# useful if we want to run individually picked tests
if __name__ == '__main__':
    unittest.main()
