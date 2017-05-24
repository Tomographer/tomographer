
import os.path
import datetime
import numpy as np
import scipy.io as sio

from make_data_file import dim, Emn, Nm, psi_W, proj_W

import tomographer
import tomographer.jpyutil
import tomographer.querrorbars

import pickle

CACHE_FILE_NAME = 'CACHE_RUN_DATA.dat'

def do_numerics():
    r = None
    with tomographer.jpyutil.RandWalkProgressBar() as prg:
        r = tomographer.tomorun.tomorun(
            dim=dim,
            Emn=Emn,
            Nm=Nm,
            jumps_method='light',
            fig_of_merit='obs-value',
            observable=proj_W,
            hist_params=tomographer.HistogramParams(0.65,0.75,50),
            mhrw_params=tomographer.MHRWParams(0.1, 50, 4096, 8*65536),
            binning_num_levels=12,
            ctrl_converged_params={'enabled': True,
                                   'max_allowed_not_converged': 3,
                                   'max_allowed_unknown': 4,
                                   'max_allowed_unknown_notisolated': 4},
            progress_fn=prg.progress_fn,
        )
        prg.displayFinalInfo(r['final_report_runs'])

    run_data = {'d': {'dim':dim,'Emn':Emn,'Nm':Nm,'psi_W':psi_W,'proj_W':proj_W},
                'r': r}
    with open(CACHE_FILE_NAME, 'wb') as f:
        pickle.dump(run_data, f, 2)

    return run_data;

if os.path.exists(CACHE_FILE_NAME):
    with open(CACHE_FILE_NAME, 'rb') as f:
        run_data = pickle.load(f)
else:
    run_data = do_numerics()
    
# data & results are in run_data
print("\nTotal computation time: {!s}".format(datetime.timedelta(seconds=run_data['r']['elapsed_seconds'])))
print(run_data['r']['final_report_runs'])

a = tomographer.querrorbars.HistogramAnalysis(run_data['r']['final_histogram'], ftox=(1,-1))
a.printFitParameters()
a.printQuantumErrorBars()
a.plot()
a.plot(log_scale=True)
