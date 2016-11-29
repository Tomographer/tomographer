
from __future__ import print_function

import logging
logging.basicConfig(level=logging.DEBUG)

import tomographer
import numpy as np

tomographer.set_log_level(logging.DEBUG)
#tlg = logging.getLogger("tomographer")
#tlg.debug("EXAMPLE MESSAGE")


Nm = np.array([ 490, 510, 494, 506, 980, 20 ])

Emn = [
    # X direction
    np.array([[1, 1],
              [1, 1]]),
    np.array([[1, -1],
              [-1, 1]]),
    # Y direction
    np.array([[1, -1j],
              [1j, 1]]),
    np.array([[1, 1j],
              [-1j, 1]]),
    # Z direction
    np.array([[1, 0],
              [0, 0]]),
    np.array([[0, 0],
              [0, 1]]),
    ]

rho_ref = np.array([[1,0],[0,0]])

print("Data ready.")

def progress(report):
    print("PROGRESS REPORT: ", report.num_completed, "/", report.num_total_runs, "\n")
    for r in report.workers_reports:
        print(repr(r))
    print("\n")

r = tomographer.tomorun.tomorun(dim=2, Nm=Nm, Emn=Emn,
                                hist_params=tomographer.UniformBinsHistogramParams(0.9,1,50),
                                mhrw_params=tomographer.MHRWParams(0.03,35,500,8192),
                                fig_of_merit="obs-value",
                                observable=rho_ref,
                                num_repeats=12,
                                progress_fn=progress)

print(repr(r))

print(r['final_histogram'].prettyPrint(100))

print(r['final_report'])
