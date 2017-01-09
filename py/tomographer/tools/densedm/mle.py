

"""
Utility to find the MLE estimate, using `cvxpy <http://www.cvxpy.org/>`_.
"""


import numpy as np

import cvxpy

import scipy.sparse as sp

import tomographer.densedm


class _Ns: pass


DEFAULT_SOLVER_OPTS = {
    'solver': cvxpy.SCS,
    'eps': 1e-6,
    'max_iters': 5000,
    'verbose': True,
} # docstring:
"""
The default solver options used by :py:func:`find_mle()`.
"""


def find_mle(llh, solver_opts=None):
    r"""
    Find the Maximum Likelihood Estimate state.  The measurement data is specified in the
    `llh` argument as a :py:class:`tomographer.densedm.IndepMeasLLH` instance.

    This function creates and solves a `cvxpy` problem solving the following:

    .. math::

         \text{maximize:}\quad &\sum_k N_k \ln\operatorname{tr}(E_k\cdot\rho)

         \text{subject to:}\quad &\rho \geqslant 0

                            &\operatorname{tr}\rho = 1

    The return value of this function is an two-tuple `(rho_MLE, d)`, where `rho_MLE` is a
    `numpy.array` representing the maximum likelihood estimate, and where `d` is an object
    with the following attributes set:

    - ``d.result``: the raw return value of `cvxpy.solve()`

    - ``d.rho_MLE``: a copy of `rho_MLE`

    - ``d.rho_vars``: a two-tuple `(rho_R, rho_I,)` of the raw `cvxpy` variables used to
      represent the complex variable `rho`. (Cvxpy doesn't support complex variables, so
      we need to declare two real matrix variables, corresponding to the real and
      imaginary parts of the variable `rho`.)

    - ``d.objective``: the raw optimization objective object constructed for `cvxpy`

    - ``d.constraints``: the list of constraints specified to `cvxpy`

    - ``d.problem``: the raw `cvxpy` problem object

    The optional `solve_opts` argument is a dictionary of options that are directly passed
    on to `cvxpy`.  The keys specified in `solve_opts` override the default keys in
    :py:const:`DEFAULT_SOLVER_OPTS`.
    """

    dim = llh.dmt.dim
    dim2 = llh.dmt.dim2
    dimtri = (dim2 - dim) / 2

    if llh.numEffects == 0:
        logging.getLogger(__name__).warning("No measurements given, did you forget to populate the llh object?")
        return None

    rho_R = cvxpy.Symmetric(dim, dim)
    rho_I = cvxpy.Variable(dim, dim)

    constraints = [
        rho_I == -rho_I.T,
        cvxpy.vstack( cvxpy.hstack(rho_R, -rho_I) ,
                      cvxpy.hstack(rho_I,  rho_R) ) >> 0,
        cvxpy.trace(rho_R) == 1,
        ]

    # TODO: go via X parameterization for the optimization ?
    #

    px = tomographer.densedm.ParamX(llh.dmt)

    Nm = llh.Nx()
    Exn = llh.Exn()
    EmnR = []
    EmnI = []
    for k in range(len(Exn)):
        # bah, it's ugly to get these back into matrices, clean this up...
        Emn = px.XToHerm(Exn[k])
        EmnR.append(Emn.real)
        EmnI.append(Emn.imag)

    print("DEBUG: EmnR = ", EmnR)
    print("DEBUG: EmnI = ", EmnI)

    objective = cvxpy.Maximize( sum( [
        Nm[k] *
        cvxpy.log( cvxpy.trace(EmnR[k] * rho_R) - cvxpy.trace(EmnI[k] * rho_I) )
        for k in range(len(Exn))
    ]) )

    problem = cvxpy.Problem(objective, constraints)

    # default solver options
    final_solver_opts = dict(DEFAULT_SOLVER_OPTS)
    if solver_opts is not None:
        # include user-given options
        final_solver_opts.update(solver_opts)

    result = problem.solve(**final_solver_opts)

    d = _Ns()
    d.result = result
    d.rho_vars = (rho_R, rho_I,)
    d.rho_MLE = rho_R.value + 1j*rho_I.value
    d.objective = objective
    d.constraints = constraints
    d.problem = problem

    if (problem.status != 'optimal'):
        d.rho_MLE = None
        print("Error: couldn't solve the MLE problem. Status = ", problem.status)

    return (d.rho_MLE, d)
