#!/usr/bin/env python

from __future__ import print_function

import re
import numpy as np
import numpy.testing as npt

import logging
logging.basicConfig(level=logging.DEBUG)

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

    def test_cxxlogger(self):
        # test that the cxxlogger object exists, and check we can set a log level.
        tomographer.cxxlogger.level = logging.INFO

    def test_exception(self):
        # test that the C++ code is able to raise an exception
        with self.assertRaises(tomographer.TomographerCxxError):
            # wrong array shape & dimensions -- causes an eigen_assert() failure
            tomographer.UniformBinsHistogram().load(np.array([ [1, 2], [3, 4] ]))




class Histograms(unittest.TestCase):
    def test_UniformBinsHistogramParams(self):
        params = tomographer.UniformBinsHistogramParams(2.0, 3.0, 5)
        self.assertAlmostEqual(params.min, 2.0)
        self.assertAlmostEqual(params.max, 3.0)
        self.assertEqual(params.num_bins, 5)
        npt.assert_array_almost_equal(params.values_lower, np.array([2.0, 2.2, 2.4, 2.6, 2.8]))
        npt.assert_array_almost_equal(params.values_upper, np.array([2.2, 2.4, 2.6, 2.8, 3.0]))
        npt.assert_array_almost_equal(params.values_center, np.array([2.1, 2.3, 2.5, 2.7, 2.9]))
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
        
    def test_UniformBinsHistogram(self):
        self.do_test_hist(tomographer.UniformBinsHistogram, int, False)

    def test_UniformBinsRealHistogram(self):
        self.do_test_hist(tomographer.UniformBinsRealHistogram, float, False)

    def test_UniformBinsHistogramWithErrorBars(self):
        self.do_test_hist(tomographer.UniformBinsHistogramWithErrorBars, float, True)

    def do_test_hist(self, HCl, cnttype, has_error_bars):
        # constructor & params member
        self.assertAlmostEqual(HCl(2.0, 3.0, 5).params.min, 2.0)
        self.assertAlmostEqual(HCl(2.0, 3.0, 5).params.max, 3.0)
        self.assertEqual(HCl(2.0, 3.0, 5).params.num_bins, 5)
        self.assertAlmostEqual(HCl(tomographer.UniformBinsHistogramParams(2.0, 3.0, 5)).params.min, 2.0)
        self.assertAlmostEqual(HCl(tomographer.UniformBinsHistogramParams(2.0, 3.0, 5)).params.max, 3.0)
        self.assertEqual(HCl(tomographer.UniformBinsHistogramParams(2.0, 3.0, 5)).params.num_bins, 5)
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
        h = HCl(tomographer.UniformBinsHistogramParams(2.0, 3.0, 5))
        npt.assert_array_almost_equal(h.bins, np.array([0,0,0,0,0]))
        self.assertAlmostEqual(h.off_chart, 0) # almost-equal in case cnttype=float
        if has_error_bars: npt.assert_array_almost_equal(h.delta, np.zeros(h.numBins()))

        # HasErrorBars is correct
        self.assertTrue(HCl.HasErrorBars == has_error_bars)

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
        with self.assertRaises(tomographer.TomographerCxxError):
            n = h.count(9999) # out of range

        # errorBar() [if error bars]
        if has_error_bars:
            h = HCl(2.0, 3.0, 5)
            h.delta = np.array([1, 2, 3, 4, 0.25])
            for i in range(h.numBins()):
                self.assertAlmostEqual(h.delta[i], h.errorBar(i))
            with self.assertRaises(tomographer.TomographerCxxError):
                n = h.errorBar(9999) # out of range

        # load()
        h = HCl(2.0, 3.0, 5)
        h.bins = np.array([1, 4, 9, 5, 2])
        h.load(np.array([10, 20, 30, 40, 50]))
        npt.assert_array_almost_equal(h.bins, np.array([10, 20, 30, 40, 50]))
        self.assertAlmostEqual(h.off_chart, 0) # almost-equal in case cnttype=float
        
        # load()
        h = HCl(2.0, 3.0, 5)
        h.bins = np.array([1, 4, 9, 5, 2])
        h.load(np.array([10, 20, 30, 40, 50]), 28)
        npt.assert_array_almost_equal(h.bins, np.array([10, 20, 30, 40, 50]))
        self.assertAlmostEqual(h.off_chart, 28) # almost-equal in case cnttype=float
        
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
        h.load(np.array([10, 20, 30, 40, 50]), 28)
        h.reset()
        npt.assert_array_almost_equal(h.bins, np.array([0,0,0,0,0]))
        self.assertAlmostEqual(h.off_chart, 0) # almost-equal in case cnttype=float

        # record()
        h = HCl(2.0, 3.0, 5)
        h.load(np.array([10, 20, 30, 40, 50]), 28)
        h.record(2.569)
        self.assertAlmostEqual(h.count(2), 31)
        h.record(2.569, cnttype(2))
        self.assertAlmostEqual(h.count(2), 33)
        h.record(2.981)
        self.assertAlmostEqual(h.count(4), 51)
        h.record(2.981, cnttype(7))
        self.assertAlmostEqual(h.count(4), 58)

        # prettyPrint()
        h = HCl(2.0, 3.0, 5)
        h.load(np.array([10, 20, 30, 40, 50]), 28)
        s = h.prettyPrint()
        print(s)
        s = h.prettyPrint(120)
        for line in s.split('\n'):
            if line:
                self.assertEqual(len(line), 120)


               

# param = tomographer.UniformBinsHistogramParams(0.0, 1.0, 10)


# basearray = np.array([0, 30, 80, 100, 200, 800, 1200, 600, 300, 50]);

# def rand_histogram():
#     u = tomographer.UniformBinsHistogram(param)
#     u.load(basearray + np.random.randint(0, 500, (10,)))
#     return u

# a = tomographer.AveragedSimpleHistogram(param)

# a.reset()
# for x in range(10):
#     a.addHistogram(rand_histogram())

# a.finalize()

# print(a.prettyPrint(80))

# print("\n".join(['\t%.4g\t%.4g'%(k,v) for k,v in zip(a.values_center, a.bins)]))




# normally, this is not needed as we are being run via pyruntest.py, but it might be
# useful if we want to run individually picked tests
if __name__ == '__main__':
    unittest.main()
