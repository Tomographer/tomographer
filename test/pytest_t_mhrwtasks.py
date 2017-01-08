#!/usr/bin/env python

from __future__ import print_function

import re
import numpy as np
import numpy.testing as npt

import logging
logging.basicConfig(level=logging.DEBUG)

import unittest

# import the module
import tomographer.mhrwtasks
import tomographer




class MHRWTasksStuff(unittest.TestCase):
    def test_fields(self):

        # just run tomorun on some arbitrary data to get some stuff to check
        mhrw_params = tomographer.MHRWParams(
            step_size=0.04,
            n_sweep=25,
            n_run=8192,
            n_therm=500)
        hist_params = tomographer.UniformBinsHistogramParams(0.985, 1, 20)
        binning_num_levels = 7
        r = tomographer.tomorun.tomorun(
            dim=2,
            Emn=[
                np.array([[0.5, -0.5j],
                          [0.5j, 0.5]]),
                np.array([[0.5, 0.5j],
                          [-0.5j, 0.5]])
            ],
            Nm=np.array([ 500, 0 ]),
            fig_of_merit="obs-value",
            observable=np.array([[0.5, -0.5j],
                                [0.5j, 0.5]]),
            num_repeats=2,
            binning_num_levels=binning_num_levels,
            mhrw_params=mhrw_params,
            hist_params=hist_params,
        )

        # check that all fields are there and display meaningful values
        
        runres = r['runs_results'][0]
        self.assertAlmostEqual(runres.mhrw_params.step_size, mhrw_params.step_size)
        self.assertEqual(runres.mhrw_params.n_sweep, mhrw_params.n_sweep)
        self.assertEqual(runres.mhrw_params.n_therm, mhrw_params.n_therm)
        self.assertEqual(runres.mhrw_params.n_run, mhrw_params.n_run)

        self.assertGreater(runres.acceptance_ratio, 0.2)
        self.assertLess(runres.acceptance_ratio, 0.4)

        stats_collector_result = runres.stats_collector_result

        self.assertEqual(stats_collector_result.hist.numBins(), hist_params.num_bins)

        npt.assert_array_equal(stats_collector_result.error_levels.shape,
                               [hist_params.num_bins, binning_num_levels+1])
        # the last error level should be the reported error bar:
        npt.assert_array_almost_equal(stats_collector_result.error_levels[:, binning_num_levels],
                                      stats_collector_result.hist.delta)

        for c in stats_collector_result.converged_status:
            self.assertIn(c, (tomographer.BinningAnalysis.CONVERGED,
                              tomographer.BinningAnalysis.NOT_CONVERGED,
                              tomographer.BinningAnalysis.UNKNOWN_CONVERGENCE) )

#


# normally, this is not needed as we are being run via pyruntest.py, but it might be
# useful if we want to run individually picked tests
if __name__ == '__main__':
    unittest.main()
