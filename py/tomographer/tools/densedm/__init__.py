
"""
This module collects utilities gravitating around some standard tasks you may need to
perform when doing quantum tomography.

A routine allows you to simulate measurement outcomes from a true quantum state and a set
of measurement settings.  The standard Pauli measurement settings on a qubit are available
in :py:const:`~tomographer.tools.densedm.PauliMeasEffectsQubit`.

A separate submodule (:py:mod:`~tomographer.tools.densedm.mle`) determines the maximum
likelihood estimate given experimental data.

Further tools may be added to these modules in the future.
"""


import numpy as np


class _Ns: pass # dummy object acts as namespace

def simulate_measurements(rho, Mk, num_samples_per_setting):
    """
    Simulate measurements for quantum tomography.

    The "true state" `rho` should be a `numpy.array` object, as well as each POVM effect
    in `Mk`.

    `Mk` should is a list of list of POVM effects, where ``Mk[k][i]`` is the POVM effect
    corresponding to outcome `i` of measurement setting `k`.  A POVM effect must be a
    positive semidefinite matrix, and all effects of a measurement setting should sum up
    to the identity matrix.
    
    `num_samples_per_setting` specifies the number of repetitions of a measurement
    setting.  If it is an integer, then we simulate this number of measurement outcomes
    for each measurement setting in `Mk`.  If it is a list (or a `numpy.array`), then it
    should have the same length as there are settings in `Mk` (i.e. ``len(Mk) ==
    len(num_samples_per_setting)``), and for the `k`-th measurement setting in `Mk` we
    will simulate ``num_samples_per_setting[k]`` number of outcomes.

    Returns: an object `d` with properties `d.Emn` and `d.Nm`, representing the POVM
    effects and simulated frequency counts.  They are in a format suitable for input to
    :py:func:`tomographer.tomorun.tomorun()` or
    :py:class:`tomographer.densedm.IndepMeasLLH`.
    """

    num_settings = len(Mk)

    num_total_effects = np.sum([ len(Mk[k]) for k in range(len(Mk)) ])

    assert len(rho.shape) == 2 and rho.shape[0] == rho.shape[1]
    
    dim = rho.shape[0]

    # prepare the list of POVM effects, and simulate the measurements
    Emn = []
    Nm = []


    if (hasattr(num_samples_per_setting, "__getitem__")):
        # a list of numbers
        numsimoutcomes = lambda k: num_samples_per_setting[k]
    else:
        # just a single constant, use same number for all measurement settings
        numsimoutcomes = lambda k: num_samples_per_setting

    for k in range(num_settings):

        # get some random numbers for this measurement setting
        #print("DEBUG: numsimoutcomes=", numsimoutcomes(k))
        x = np.random.rand(numsimoutcomes(k))
        proboffset = 0

        # We sample the measurement outcomes as follows: we split the interval [0,1]
        # in number sections equal to the number of possible outcomes, each of length
        # = probability of that outcome.  Then, for each possible outcome, we count
        # the number of random numbers in `x` that fall into the corresponding
        # section.

        for i in range(len(Mk[k])):

            Ek = Mk[k][i]
            p = np.einsum('ij,ji->', Ek, rho) # = trace(Ek * rho)
            Emn.append( Ek.astype(dtype=complex) )
            Nm.append( np.count_nonzero( (proboffset <= x) & (x < proboffset+p) ) )

            proboffset += p

        # sanity check
        assert np.abs(proboffset-1.0) < 1e-4, (
            "POVM effects do not sum to identity or rho_sim not normalized, got proboffset={}".format(proboffset)
            )

    d = _Ns()
    d.Emn = Emn
    d.Nm = np.array(Nm)
    return d





#
# Measurement settings
#

PauliMeasEffectsQubit = [ [
        np.array([[.5, .5],[.5, .5]]),     # X, +1 outcome
        np.array([[.5, -.5],[-.5, .5]]),   # X, -1 outcome
    ], [
        np.array([[.5, -.5j],[.5j, .5]]),  # Y, +1 outcome
        np.array([[.5, .5j],[-.5j, .5]]),  # Y, -1 outcome
    ], [
        np.array([[1,0],[0,0]]),           # Z, +1 outcome
        np.array([[0,0],[0,1]]),           # Z, -1 outcome
    ]
] # docstring:
"""
List of POVMs corresponding to measuring Pauli operators on a single qubit.
``PauliMeasEffectQubit[i][s]`` is the POVM effect corresponding to measuring the outcome
indexed by `s` of the Pauli operator indexed by `i`.
"""
