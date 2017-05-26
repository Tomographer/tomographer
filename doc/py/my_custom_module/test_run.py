
from __future__ import print_function

import logging
logging.basicConfig(level=logging.DEBUG)
logger = logging.getLogger(__name__)

import numpy as np

import my_custom_module
import tomographer

#
# Some minimal example data
#

Emn = [
    np.array([[1, 0],
              [0, 0]]),
    np.array([[0, 0],
              [0, 1]])
    ]
Nm = np.array([ 98, 2 ])


r = my_custom_module.run(
    2, Emn=Emn, Nm=Nm,
    mhrw_params=tomographer.MHRWParams(step_size=0.01, n_sweep=100, n_therm=256, n_run=8192),
    hist_params=tomographer.HistogramParams(min=0.8, max=1.0, num_bins=40),
    progress_fn=lambda report: print(report.getHumanReport()),
)

print(r['final_report'])
