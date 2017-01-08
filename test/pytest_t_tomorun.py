#!/usr/bin/env python

from __future__ import print_function

import re
import numpy as np
import numpy.testing as npt

import logging
#logging.basicConfig(level=logging.DEBUG)

import unittest

# import the module
import tomographer.tomorun
import tomographer.densedm
import tomographer



class AnalyticalSolutionFn(object):
    # see the cxx file test/tomorun/test_tomorun_1qubit_analytic_solution_check.cxx

    def __init__(self, n):
        self.n = n
        #self.c = 2.0 / (15.0 + 16.0*n + 4.0*n*n)
        self.lnc = np.log(2.0) - np.log(15.0 + 16.0*n + 4.0*n*n)

    def lnvalue(self, f):
        return 2*np.log(f) + np.log(1+f) + np.log(1-f) + 2 * self.n * np.log(f) - self.lnc;

    def get_histogram_chi2_red(self, hist):

        ln_bin_delta = np.log(hist.params.binResolution())

        sumwsqdiff = 0
        numpts = 0

        for k in range(hist.numBins()):
            # ignore (near-)zero data points
            if hist.count(k) < 1e-12:
                continue

            fval = hist.params.binCenterValue(k)
            valln = np.log(hist.count(k))
            errln = hist.errorBar(k) / hist.count(k)

            # calculate squared difference to expected theoretical value, and weigh by 1/err
            theo_valln = self.lnvalue(fval) + ln_bin_delta;

            sumwsqdiff += ((valln - theo_valln) / errln) ** 2
            numpts += 1

            #print("Point {:03d}: theoln={:.3g} valln={:.3g} errln={:.3g}".format(numpts, theo_valln, valln, errln))
           
        chi2_red = sumwsqdiff / (numpts - 1)
        print("chi2_red={:.4g} ; sumwsqdiff={:.3g}, numpts={}".format(chi2_red, sumwsqdiff, numpts))
        return chi2_red


class analytical_known_example_tomorun(unittest.TestCase):

    def setUp(self):

        self.Emn = [
            # +Y
            np.array([[0.5, -0.5j],
                      [0.5j, 0.5]]),
            # -Y
            np.array([[0.5, 0.5j],
                      [-0.5j, 0.5]])
        ]
        # tomography data: 500x counts +1, 0x counts -1; when measuring sigma_Y 500 times in total
        self.Nm = np.array([ 500, 0 ])

        # reference state: +Y
        self.rho_ref = np.array([[0.5, -0.5j],
                                 [0.5j, 0.5]])


    def test_values(self):

        print("test_values()")

        num_repeats = 8
        hist_params = tomographer.UniformBinsHistogramParams(0.985, 1, 200)

        r = tomographer.tomorun.tomorun(
            dim=2,
            Emn=self.Emn,
            Nm=self.Nm,
            fig_of_merit="fidelity",
            ref_state=self.rho_ref,
            num_repeats=num_repeats,
            mhrw_params=tomographer.MHRWParams(
                step_size=0.04,
                n_sweep=25,
                n_run=8192,
                n_therm=500),
            hist_params=hist_params,
        )
        print("Final report of runs :\n{}".format(r['final_report_runs']))
        print("Final report of everything :\n{}".format(r['final_report']))

        final_histogram = r['final_histogram']
        self.assertTrue(isinstance(final_histogram, tomographer.AveragedErrorBarHistogram))
        simple_final_histogram = r['simple_final_histogram']
        self.assertTrue(isinstance(simple_final_histogram, tomographer.AveragedSimpleRealHistogram))

        print("Tomorun completed in {} seconds".format(r['elapsed_seconds']))

        for k in range(num_repeats):
            runres = r['runs_results'][k]
            self.assertTrue(isinstance(runres, tomographer.mhrwtasks.MHRandomWalkValueHistogramTaskResult))

        # now, check the actual values of the result
        pok = AnalyticalSolutionFn(np.sum(self.Nm))

        self.assertLess(pok.get_histogram_chi2_red(final_histogram), 1.5)
        npt.assert_array_almost_equal(final_histogram.bins, simple_final_histogram.bins)


    def test_errbar_convergence(self):

        print("test_errbar_convergence()")

        num_repeats = 2
        hist_params = tomographer.UniformBinsHistogramParams(0.985, 1, 20)

        r = tomographer.tomorun.tomorun(
            dim=2,
            Emn=self.Emn,
            Nm=self.Nm,
            fig_of_merit="fidelity",
            ref_state=self.rho_ref,
            num_repeats=num_repeats,
            mhrw_params=tomographer.MHRWParams(
                step_size=0.04,
                n_sweep=25,
                n_run=16*32768,
                n_therm=500),
            hist_params=hist_params,
        )

        print("Final report of runs :\n{}".format(r['final_report_runs']))

        # inspect the task runs
        for k in range(num_repeats):
            runres = r['runs_results'][k]
            # check that at most 15% of the error bars have not converged
            self.assertGreater( (runres.stats_collector_result.converged_status == 
                                 np.full([hist_params.num_bins],
                                         tomographer.BinningAnalysis.CONVERGED, dtype=int)).sum(),
                                0.85*hist_params.num_bins )


    def test_callback(self):

        print("test_callback()")

        num_repeats = 2
        hist_params = tomographer.UniformBinsHistogramParams(0.985, 1, 200)

        class Ns: pass

        glob = Ns()
        glob.num_callback_calls = 0

        def progress_callback(fullstatusreport):
            glob.num_callback_calls += 1
            print(fullstatusreport.getHumanReport())

        intvl_ms = 200

        r = tomographer.tomorun.tomorun(
            dim=2,
            Emn=self.Emn,
            Nm=self.Nm,
            fig_of_merit="obs-value",
            observable=self.rho_ref,
            num_repeats=num_repeats,
            mhrw_params=tomographer.MHRWParams(
                step_size=0.04,
                n_sweep=25,
                n_run=4*32768,
                n_therm=500),
            hist_params=hist_params,
            progress_fn=progress_callback,
            progress_interval_ms=intvl_ms,
        )

        # we have the total elapsed time in r['elapsed_seconds']

        print("Total elapsed: {:.2g} seconds".format(r['elapsed_seconds']))

        nc = 1000*r['elapsed_seconds']/intvl_ms
        self.assertGreaterEqual(glob.num_callback_calls, 1)
        self.assertLessEqual(glob.num_callback_calls, nc+2)



# normally, this is not needed as we are being run via pyruntest.py, but it might be
# useful if we want to run individually picked tests
if __name__ == '__main__':
    unittest.main()
