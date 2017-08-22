
import numpy as np

import logging
#logging.basicConfig(level=logging.DEBUG) # uncomment for debug messages

# import Tomographer & utilities
import tomographer
import tomographer.tomorun
import tomographer.jpyutil
import tomographer.querrorbars

r = None
with tomographer.jpyutil.RandWalkProgressBar() as prg:
    r = tomographer.tomorun.tomorun(
        dim=2,
        # POVM effects and frequencies -- Pauli measurements on a qubit, 500x/axis
        Emn=[ np.array([[.5, .5],
                        [.5, .5]]), # +X
              np.array([[.5, -.5],
                        [-.5, .5]]), # -X
              np.array([[.5, -.5j],
                        [.5j, .5]]), # +Y
              np.array([[.5, .5j],
                        [-.5j, .5]]), # -Y
              np.array([[1, 0],
                        [0, 0]]), # +Z
              np.array([[0, 0],
                        [0, 1]]) ], # -Z
        Nm=np.array([262, 238,  231, 269,  483, 17]), # plug in real data here
        # figure of merit: fidelity-squared to pure |0> state
        fig_of_merit="obs-value",
        observable=np.array([[1, 0], [0, 0]]),
        # histogram parameters
        hist_params=tomographer.HistogramParams(min=0.9,max=1.0,num_bins=40),
        # settings of the random walk
        mhrw_params=tomographer.MHRWParams(
            step_size=0.035, # adjust such that accept. ratio ~ 0.25
            n_sweep=30, # set such that step_size*n_sweep ~ 1
            n_therm=500,
            n_run=32768),
        # our progress callback
        progress_fn=prg.progress_fn
    )
    # Check the final report of runs to make sure error bars have (mostly all) converged,
    # and to verify the acceptance ratio is more or less in the range [0.2, 0.4]
    prg.displayFinalInfo(r['final_report_runs'])

# The final histogram, as a `tomographer.AveragedErrorBarHistogram` instance:
final_histogram = r['final_histogram']


# For a quick visual text-based representation of the histogram, use prettyPrint():
#
#print("Finished. Here is the final histogram:")
#print(final_histogram.prettyPrint())


# Do the analysis and determine the quantum error bars
analysis = tomographer.querrorbars.HistogramAnalysis(
    final_histogram,
    ftox=(1,-1), # x=1-f for fidelity
)
analysis.printFitParameters()
q = analysis.printQuantumErrorBars() # q has attributes: q.f0, q.Delta, q.gamma
# show linear scale plot
analysis.plot()
# log scale plot (adjust scale before showing plot)
p = analysis.plot(log_scale=True, show_plot=False)
p.ax.set_ylim([1e-5, 1e2])
p.show()
