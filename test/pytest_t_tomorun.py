#!/usr/bin/env python

from __future__ import print_function

#import sys
import re
import numpy as np
import numpy.linalg as npl
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


    def test_values_full(self):

        print("test_values_full()")

        num_repeats = 8
        hist_params = tomographer.HistogramParams(0.985, 1, 200)

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
                n_run=32768,
                n_therm=500),
            jumps_method="full",
            hist_params=hist_params,
            progress_fn=lambda report: print(report.getHumanReport()),
            progress_interval_ms=500,
            ctrl_converged_params={'max_allowed_not_converged': 1,
                                   'max_allowed_unknown_notisolated': 1,
                                   'max_allowed_unknown': 3,}
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
            self.assertTrue(isinstance(runres, tomographer.mhrwtasks.MHRandomWalkTaskResult))

        # now, check the actual values of the result
        pok = AnalyticalSolutionFn(np.sum(self.Nm))

        self.assertLess(pok.get_histogram_chi2_red(final_histogram), 1.5)
        npt.assert_array_almost_equal(final_histogram.bins, simple_final_histogram.bins)


    def test_values_light(self):

        print("test_values_light()")

        num_repeats = 8
        hist_params = tomographer.HistogramParams(0.985, 1, 200)

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
                n_run=32768,
                n_therm=500),
            jumps_method="light",
            hist_params=hist_params,
            progress_fn=lambda report: print(report.getHumanReport()),
            progress_interval_ms=500,
            ctrl_converged_params={'max_allowed_not_converged': 1,
                                   'max_allowed_unknown_notisolated': 1,
                                   'max_allowed_unknown': 3,}
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
            self.assertTrue(isinstance(runres, tomographer.mhrwtasks.MHRandomWalkTaskResult))

        # now, check the actual values of the result
        pok = AnalyticalSolutionFn(np.sum(self.Nm))

        self.assertLess(pok.get_histogram_chi2_red(final_histogram), 1.5)
        npt.assert_array_almost_equal(final_histogram.bins, simple_final_histogram.bins)


    def test_errbar_convergence(self):

        print("test_errbar_convergence()")

        num_repeats = 2
        hist_params = tomographer.HistogramParams(0.995, 1, 100)

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
                n_run=8192, # controller will keep running as necessary
                n_therm=500),
            hist_params=hist_params,
            ctrl_converged_params={'max_allowed_unknown': 2,
                                   'max_allowed_unknown_notisolated': 2,
                                   'max_allowed_not_converged': 0,
                                   # run as long as necessary
                                   'max_add_run_iters': -1},
            progress_fn=lambda report: print(report.getHumanReport()),
            progress_interval_ms=50
        )

        print("Final report of runs :\n{}".format(r['final_report_runs']))

        # inspect the task runs
        for k in range(num_repeats):
            runres = r['runs_results'][k]
            # check that bins have converged as required
            self.assertLessEqual(
                (runres.stats_results.converged_status == 
                 tomographer.BinningAnalysis.NOT_CONVERGED*np.ones([hist_params.num_bins],
                                                                   dtype=int)).sum(),
                0

            )
            self.assertLessEqual(
                (runres.stats_results.converged_status == 
                 tomographer.BinningAnalysis.UNKNOWN_CONVERGENCE*np.ones([hist_params.num_bins],
                                                                         dtype=int)).sum(),
                2
            )


    def test_custom_figofmerit(self):

        print("test_custom_figofmerit()")

        num_repeats = 2
        hist_params = tomographer.HistogramParams(0.99, 1, 20)

        r = tomographer.tomorun.tomorun(
            dim=2,
            Emn=self.Emn,
            Nm=self.Nm,
            fig_of_merit=lambda T: npl.norm(np.dot(T,T.T.conj())), # purity
            ref_state=self.rho_ref,
            num_repeats=num_repeats,
            mhrw_params=tomographer.MHRWParams(
                step_size=0.04,
                n_sweep=25,
                n_run=8192,
                n_therm=500),
            hist_params=hist_params,
            progress_fn=lambda report: print(report.getHumanReport()),
            progress_interval_ms=100
        )

        print(r['final_report'])
        # just make sure that less than 1% of points are out of [0.99,1]
        self.assertLess(r['final_histogram'].off_chart, 0.01)


    def test_custom_figofmerit_parallel(self):

        print("test_custom_figofmerit_parallel()")

        num_repeats = 8
        hist_params = tomographer.HistogramParams(0.99, 1, 20)

        mhrw_params = tomographer.MHRWParams(
            step_size=0.04,
            n_sweep=25,
            n_run=8192,
            n_therm=500
        )

        class Ns: pass
        glob = Ns()
        glob.saw_parallel_runs = False

        def is_running(w):
            if w is None:
                return False
            return w.data['kstep'] > (mhrw_params.n_therm+2)*mhrw_params.n_sweep

        def prg_fn(report):
            print(report.getHumanReport())
            num_running = sum([ (1 if is_running(w) else 0)  for w in report.workers ])
            if num_running > 1:
                glob.saw_parallel_runs = num_running
                raise Exception("Done, test passed.")

        try:
            r = tomographer.tomorun.tomorun(
                dim=2,
                Emn=self.Emn,
                Nm=self.Nm,
                fig_of_merit=lambda T: npl.norm(np.dot(T,T.T.conj())), # purity
                ref_state=self.rho_ref,
                num_repeats=num_repeats,
                mhrw_params=mhrw_params,
                hist_params=hist_params,
                progress_fn=prg_fn,
                progress_interval_ms=50,
                ctrl_step_size_params={'enabled':False},
            )
        except Exception as e:
            if 'Done, test passed' not in str(e):
                raise

        self.assertGreaterEqual(glob.saw_parallel_runs, 2)


    def test_too_few_runs(self):

        print("test_too_few_runs()")
        num_repeats = 2
        hist_params = tomographer.HistogramParams(0.99, 1, 10)

        r = tomographer.tomorun.tomorun(
            dim=2,
            Emn=self.Emn,
            Nm=self.Nm,
            fig_of_merit="fidelity",
            ref_state=self.rho_ref,
            num_repeats=num_repeats,
            mhrw_params=tomographer.MHRWParams(0.04, 25, 500, 100), # 100 is too few runs
            hist_params=hist_params,
            ctrl_step_size_params={'enabled':False},
            ctrl_converged_params={'enabled':False},
        )


    def test_mhwalker_param_1(self):

        print("test_mhwalker_param_1()")
        num_repeats = 2
        hist_params = tomographer.HistogramParams(0.99, 1, 10)
        r = tomographer.tomorun.tomorun(
            dim=2,
            Emn=self.Emn,
            Nm=self.Nm,
            fig_of_merit="fidelity",
            ref_state=self.rho_ref,
            num_repeats=num_repeats,
            # understands step size given as positional argument? ---
            mhrw_params=tomographer.MHRWParams(0.04, 25, 500, 1024),
            # ---
            hist_params=hist_params,
            ctrl_step_size_params={'enabled':False},
            ctrl_converged_params={'enabled':False},
        )
        print(r['final_report'])
        for rw in r['runs_results']:
            self.assertAlmostEqual(rw.mhrw_params.mhwalker_params["step_size"], 0.04)

    def test_mhwalker_param_2(self):

        print("test_mhwalker_param_2()")
        num_repeats = 2
        hist_params = tomographer.HistogramParams(0.99, 1, 10)
        r = tomographer.tomorun.tomorun(
            dim=2,
            Emn=self.Emn,
            Nm=self.Nm,
            fig_of_merit="fidelity",
            ref_state=self.rho_ref,
            num_repeats=num_repeats,
            # understands step size given as keyword argument? ---
            mhrw_params=tomographer.MHRWParams(step_size=0.04, n_sweep=25, n_therm=500, n_run=1024),
            # ---
            hist_params=hist_params,
            ctrl_step_size_params={'enabled':False},
            ctrl_converged_params={'enabled':False},
        )
        print(r['final_report'])
        for rw in r['runs_results']:
            self.assertAlmostEqual(rw.mhrw_params.mhwalker_params["step_size"], 0.04)

    def test_mhwalker_param_3(self):

        print("test_mhwalker_param_3()")
        num_repeats = 2
        hist_params = tomographer.HistogramParams(0.99, 1, 10)
        r = tomographer.tomorun.tomorun(
            dim=2,
            Emn=self.Emn,
            Nm=self.Nm,
            fig_of_merit="fidelity",
            ref_state=self.rho_ref,
            num_repeats=num_repeats,
            # understands step size given as dictionary? ---
            mhrw_params=tomographer.MHRWParams({'step_size': 0.04}, 25, 500, 1024),
            # ---
            hist_params=hist_params,
            ctrl_step_size_params={'enabled':False},
            ctrl_converged_params={'enabled':False},
        )
        print(r['final_report'])
        for rw in r['runs_results']:
            self.assertAlmostEqual(rw.mhrw_params.mhwalker_params["step_size"], 0.04)

    def test_mhwalker_param_4(self):

        print("test_mhwalker_param_4()")
        num_repeats = 2
        hist_params = tomographer.HistogramParams(0.99, 1, 10)
        r = tomographer.tomorun.tomorun(
            dim=2,
            Emn=self.Emn,
            Nm=self.Nm,
            fig_of_merit="fidelity",
            ref_state=self.rho_ref,
            num_repeats=num_repeats,
            # understands 'None'? ---
            mhrw_params=tomographer.MHRWParams(None, 25, 500, 1024),
            # ---
            hist_params=hist_params,
            ctrl_step_size_params={'enabled':True}, # auto-adjust
            ctrl_converged_params={'enabled':False},
        )
        print(r['final_report'])
        for rw in r['runs_results']:
            self.assertLessEqual(rw.mhrw_params.mhwalker_params["step_size"], 0.1)
            self.assertGreaterEqual(rw.mhrw_params.mhwalker_params["step_size"], 0.005)

    def test_mhwalker_param_5(self):

        print("test_mhwalker_param_5()")
        num_repeats = 2
        hist_params = tomographer.HistogramParams(0.99, 1, 10)
        r = tomographer.tomorun.tomorun(
            dim=2,
            Emn=self.Emn,
            Nm=self.Nm,
            fig_of_merit="fidelity",
            ref_state=self.rho_ref,
            num_repeats=num_repeats,
            # understands missing key? ---
            mhrw_params=tomographer.MHRWParams({}, 25, 500, 1024),
            # ---
            hist_params=hist_params,
            ctrl_step_size_params={'enabled':True}, # auto-adjust
            ctrl_converged_params={'enabled':False},
        )
        print(r['final_report'])
        for rw in r['runs_results']:
            self.assertLessEqual(rw.mhrw_params.mhwalker_params["step_size"], 0.1)
            self.assertGreaterEqual(rw.mhrw_params.mhwalker_params["step_size"], 0.005)


    def test_callback(self):

        print("test_callback()")

        num_repeats = 2
        hist_params = tomographer.HistogramParams(0.985, 1, 200)

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
            ctrl_converged_params={'enabled': False},
        )

        # we have the total elapsed time in r['elapsed_seconds']

        print("Total elapsed: {:.2g} seconds".format(r['elapsed_seconds']))

        nc = 1000*r['elapsed_seconds']/intvl_ms
        self.assertGreaterEqual(glob.num_callback_calls, 1)
        self.assertLessEqual(glob.num_callback_calls, nc+2)

    def test_error_in_callback(self):

        print("test_error_in_callback()")

        num_repeats = 2
        hist_params = tomographer.HistogramParams(0.985, 1, 200)

        class Ns: pass

        #
        # Make sure an error in the callback raises an Exception
        #
        def progress_callback(fullstatusreport):
            error-xxx(xyz) # error -- raises a Python exception
            print(fullstatusreport.getHumanReport())

        intvl_ms = 200

        with self.assertRaises(Exception):
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


    def test_convergence_control_1(self):

        print("test_convergence_control_1()")

        num_repeats = 2
        hist_params = tomographer.HistogramParams(0.985, 1, 50)

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
                n_run=1024, # ridicoulously low
                n_therm=200),
            hist_params=hist_params,
            progress_fn=lambda report: print(report.getHumanReport()),
            progress_interval_ms=100,
            ctrl_converged_params={'enabled': True,
                                   'max_allowed_unknown': 1,
                                   'max_allowed_unknown_notisolated': 1,
                                   'max_allowed_not_converged': 1,
                                   # run as long as is necessary
                                   'max_add_run_iters': -1 },
        )

        for runres in r['runs_results']:
            summary = runres.stats_results.errorBarConvergenceSummary()
            self.assertLessEqual(summary.n_unknown, 1)
            self.assertLessEqual(summary.n_unknown-summary.n_unknown_isolated, 1)
            self.assertLessEqual(summary.n_not_converged, 1)

    def test_convergence_control_2(self):

        print("test_convergence_control_2()")

        num_repeats = 2
        hist_params = tomographer.HistogramParams(0.985, 1, 50)

        class Ns: pass
        glob = Ns()
        glob.num_callback_calls = 0

        def check_prg(report):
            glob.num_callback_calls += 1
            for r in report.workers:
                if r is not None and r.fraction_done > 1:
                    raise AssertionError("Control is disabled, random walk should not go past 100%")

        r = tomographer.tomorun.tomorun(
            dim=2,
            Emn=self.Emn,
            Nm=self.Nm,
            fig_of_merit="obs-value",
            observable=self.rho_ref,
            num_repeats=num_repeats,
            mhrw_params=tomographer.MHRWParams(
                step_size=0.1, # too high
                n_sweep=1000,
                n_run=8192, # some ok amount, not too much
                n_therm=100),
            hist_params=hist_params,
            progress_fn=check_prg,
            progress_interval_ms=1,
            ctrl_converged_params={'enabled': False},
            # keep step size a little off, to prevent bins from converging too nicely:
            ctrl_step_size_params={'enabled': False},
        )

        # make sure progress reporter was called often enough
        self.assertGreaterEqual(glob.num_callback_calls, 10)


    def test_stepsize_control_1(self):

        print("test_stepsize_control_1()")

        num_repeats = 2
        hist_params = tomographer.HistogramParams(0.985, 1, 50)

        r = tomographer.tomorun.tomorun(
            dim=2,
            Emn=self.Emn,
            Nm=self.Nm,
            fig_of_merit="obs-value",
            observable=self.rho_ref,
            num_repeats=num_repeats,
            mhrw_params=tomographer.MHRWParams(
                step_size=0.5, # must be adjusted
                n_sweep=2,
                n_run=1024,
                n_therm=1),
            hist_params=hist_params,
            progress_fn=lambda report: print(report.getHumanReport()),
            progress_interval_ms=100,
            ctrl_step_size_params={'enabled': True},
            ctrl_converged_params={'enabled': False},
        )

        for runres in r['runs_results']:
            print("Step size is ",runres.mhrw_params.mhwalker_params['step_size'])
            print("and n_therm is ",runres.mhrw_params.n_therm)
            self.assertLessEqual( runres.mhrw_params.mhwalker_params['step_size'], 0.1 )
            self.assertGreaterEqual( runres.mhrw_params.mhwalker_params['step_size'], 0.01 )
            self.assertGreaterEqual( runres.acceptance_ratio, 0.2 )
            self.assertLessEqual( runres.acceptance_ratio, 0.4 )
            self.assertGreaterEqual( runres.mhrw_params.n_therm , 20 ) # make sure it was adjusted

    def test_stepsize_control_2(self):

        print("test_stepsize_control_2()")

        num_repeats = 2
        hist_params = tomographer.HistogramParams(0.985, 1, 50)

        orig_step_size = 0.5
        orig_n_therm = 2

        r = tomographer.tomorun.tomorun(
            dim=2,
            Emn=self.Emn,
            Nm=self.Nm,
            fig_of_merit="obs-value",
            observable=self.rho_ref,
            num_repeats=num_repeats,
            mhrw_params=tomographer.MHRWParams(
                step_size=orig_step_size, # must be adjusted
                n_sweep=2,
                n_run=1024,
                n_therm=orig_n_therm),
            hist_params=hist_params,
            progress_fn=lambda report: print(report.getHumanReport()),
            progress_interval_ms=100,
            ctrl_step_size_params={'enabled': False},
            ctrl_converged_params={'enabled': False},
        )

        for runres in r['runs_results']:
            print("Step size is ",runres.mhrw_params.mhwalker_params['step_size'])
            print("and n_therm is ",runres.mhrw_params.n_therm)
            # make sure it wasn't adjusted
            self.assertAlmostEqual( runres.mhrw_params.mhwalker_params['step_size'], orig_step_size )
            self.assertEqual( runres.mhrw_params.n_therm, orig_n_therm )




# normally, this is not needed as we are being run via pyruntest.py, but it might be
# useful if we want to run individually picked tests
if __name__ == '__main__':
    unittest.main()
