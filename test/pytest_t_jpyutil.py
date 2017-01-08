
import time

import tomographer.jpyutil
import tomographer
import tomographer.tomorun

import numpy as np

import unittest

class ProgressBarsTest(unittest.TestCase):

    def test_simple_progress(self):

        with tomographer.jpyutil.SimpleProgressBar("progress in progress") as prg:
            for i in range(100):
                prg.progress(fraction_done=i/100.0)
                time.sleep(0.02)

    def test_rand_walk(self):
        
        r = None
        with tomographer.jpyutil.RandWalkProgressBar("random walk in progress") as prg:
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
                progress_fn=prg.progress_fn,
            )
            prg.displayFinalInfo(r['final_report_runs'])




# normally, this is not needed as we are being run via pyruntest.py, but it might be
# useful if we want to run individually picked tests
if __name__ == '__main__':
    unittest.main()
