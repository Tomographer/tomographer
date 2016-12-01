
from __future__ import print_function

import logging
logging.basicConfig(level=logging.INFO)

import tomographer
import numpy as np

#tomographer.cxxlogger.level = logging.DEBUG
#tomographer.cxxlogger.level = logging.NOTSET
#tlg = logging.getLogger("tomographer")
#tlg.debug("EXAMPLE MESSAGE")

#logging.getLogger("tomographer").setLevel(logging.INFO)


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
    print("PROGRESS REPORT: ", report.num_completed, "/", report.num_total_runs, "   ", report.elapsed, "s elapsed")
    for r in report.workers:
        if not r:
            print("  :: (idle thread)")
            continue
        #print("  {worker_id:2d}: {fraction_done:.2f%}  accept={acceptance_ratio:.2f} \n".format(**r))
        print("    {worker_id:02d}: {fraction_done:.2%}  accept={acceptance_ratio:.2f}".format(
            worker_id=r.worker_id, fraction_done=r.fraction_done, **r.data))
        print("        {}".format(r.msg.replace('\n', '\n    ')))
    print()
def progress2(report):
    print(report.getHumanReport())

try:
    r = tomographer.tomorun.tomorun(dim=2, Nm=Nm, Emn=Emn,
                                    hist_params=tomographer.UniformBinsHistogramParams(0.9,1,50),
                                    mhrw_params=tomographer.MHRWParams(0.03,35,500,8192),
                                    fig_of_merit="obs-value",
                                    observable=rho_ref,
                                    num_repeats=12,
                                    progress_fn=progress2)
    print(repr(r))

    print(r['final_histogram'].prettyPrint(100))

    print(r['final_report'])

except KeyboardInterrupt as e:
    print("INTERRUPTED!")
    import traceback
    traceback.print_exc()

