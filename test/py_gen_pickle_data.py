#
# Generated pickled data for current tomographer version, to make sure that data can be
# loaded with backwards compatibility.
#

from __future__ import print_function

import sys
import os
import os.path

import numpy as np

import logging
logging.basicConfig(level=logging.DEBUG)

import tomographer
import tomographer.mhrwtasks
import tomographer.densedm
import tomographer.multiproc

import pickle

version = tomographer.__version__
tveri = tomographer.version.version_info

pyversion = sys.version_info[0]

pickledatadirroot = os.path.join(os.path.dirname(os.path.realpath(os.path.abspath(__file__))),
                                 '_pickledata-py{}'.format(pyversion))

if not os.path.isdir(pickledatadirroot):
    os.mkdir(pickledatadirroot)

pickledatadir = os.path.join(pickledatadirroot, version)

if not os.path.isdir(pickledatadir):
    os.mkdir(pickledatadir)


print("Pickling data to directory '{}', this is Tomographer version {} [{!r}]".format(pickledatadir, version, tveri))


# tomographer: Histogram classes
def do_histogram():

    d = {}
    
    if tveri < (5,0):
        # Tomographer < 5.0
        p = tomographer.UniformBinsHistogramParams(0.0, 1.0, 5)
        d["UniformBinsHistogramParams"] = p
    else:
        # Tomographer >= 5.0
        p = tomographer.HistogramParams(0.0, 1.0, 5)
        d["HistogramParams"] = p

    def load_values_maybe_error_bars(h, values, errors, off_chart=0):
        if not h.has_error_bars:
            h.load(values, off_chart)
        else:
            h.load(values, errors, off_chart)

    if tveri < (5,0):
        # Tomographer < 5.0
        klasses = [(tomographer.UniformBinsHistogram, 'UniformBinsHistogram'),
                     (tomographer.UniformBinsRealHistogram, 'UniformBinsRealHistogram'),
                     (tomographer.UniformBinsHistogramWithErrorBars, 'UniformBinsHistogramWithErrorBars'),
                     (tomographer.AveragedSimpleHistogram, 'AveragedSimpleHistogram'),
                     (tomographer.AveragedSimpleRealHistogram, 'AveragedSimpleRealHistogram'),
                     (tomographer.AveragedErrorBarHistogram, 'AveragedErrorBarHistogram'),]
    else:
        # Tomographer >= 5.0
        klasses = [(tomographer.Histogram, 'Histogram'),
                   (tomographer.HistogramWithErrorBars, 'HistogramWithErrorBars'),]

    for c, n in klasses:
                 
        x = c(p)
        load_values_maybe_error_bars(x, np.array([10, 20, 30, 40, 50]),
                                     np.array([1, 2, 3, 4, 5]), 28)

        d[n] = x

    # save all of this stuff as a pickle
    with open(os.path.join(pickledatadir, 'histograms.pickle'), 'wb') as f:
        pickle.dump(d,f,2)

do_histogram()


# tomographer.densedm: LLH class
def do_densedm():

    if tveri < (5,0):
        # Pickling in tomographer.densedm was broken before Tomographer 5.0
        return

    # Following for Tomographer >= 5.0

    d = {}

    dmt = tomographer.densedm.DMTypes(dim=2)
    llh = tomographer.densedm.IndepMeasLLH(dmt)
    llh.setMeas(np.array([ [1, 0, 0, 0], [0, 1, 0, 0] ]), np.array([15, 85]))

    d['llh'] = llh

    # save all of this stuff as a pickle
    with open(os.path.join(pickledatadir, 'densedm.pickle'), 'wb') as f:
        pickle.dump(d,f,2)
    
do_densedm()


# tomographer.tomorun: Task results & reports
def do_tomorun():

    d = {}

    d['result'] = tomographer.tomorun.tomorun(
        dim=2,
        Emn=[
            # +Y
            np.array([[0.5, -0.5j],
                      [0.5j, 0.5]]),
            # -Y
            np.array([[0.5, 0.5j],
                      [-0.5j, 0.5]])
        ],
        Nm=np.array([ 423, 87 ]),
        fig_of_merit="obs-value",
        observable=np.array([[0.5, -0.5j],
                             [0.5j, 0.5]]),
        mhrw_params=tomographer.MHRWParams(
            step_size=0.1,
            n_sweep=10,
            n_run=32768,
            n_therm=500),
        hist_params=tomographer.UniformBinsHistogramParams(0.7, 0.9, 20),
        progress_fn=lambda x: print(x.getHumanReport())
    )

    # save all of this stuff as a pickle
    with open(os.path.join(pickledatadir, 'tomorun.pickle'), 'wb') as f:
        pickle.dump(d,f,2)
    
do_tomorun()
