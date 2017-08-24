
"""
Utilities for fitting the histogram data and computing the *quantum error bars*.
"""

from __future__ import print_function

import collections # namedtuple
import inspect
import numpy as np
import scipy
import scipy.optimize as sciopt

import logging
logger=logging.getLogger(__name__)


class _Ns:
    pass




def fit_fn_a2(x, a2, a1, m, c):
    """
    A standard fit model for the logarithm of the histogram data.  The
    `x`-values are not directly the figure of merit, but may be possibly
    transformed via a `f`-to-`x` transformation (see
    :py:class:`HistogramAnalysis`).

    Returns the value

    .. math::
    
        y(x) = -a_2\\,x^2 - a_1\\,x + m\\log(x) + c \\ ,

    which is the default fit model.  If a `NumPy` array is specified as `x`, the
    function is evaluated for all the given values and the values are returned
    as a vector.

    .. versionadded:: 5.2

    """
    return -a2*np.square(x) - a1*x + m*np.log(x) + c


def fit_fn_default(x, a2, a1, m, c):
    """
    The default fit model.  This is currently an alias for :py:func:`fit_fn_a2`,
    but this might change in a future version of `tomographer`.
    """
    return fit_fn_a2(x, a2, a1, m, c)


def zm(w, m):
    """
    Returns the "skewing" portion of :math:`y(x)`.

    The function is defined as

    .. math::
        z_m(w) = \\frac{m}{2} w^2 - 2m\\,w + m \\ln w + \\frac{3m}{2} \\ ,

    and relates to the difference of our fit model with its deskewed version as
    :math:`y(x) - y_{\\mathrm{deskewed}}(x) = z_m(x/x_0)`.

    .. versionadded:: 5.2

    """
    return m*np.square(w)/2 - 2*m*w + m*np.log(w) + 3*m/2

def fit_fn_direct(x, a, x0, y0, m):
    """
    The new "direct" fit model for the logarithm of the histogram data.  The `x`-values
    are not directly the figure of merit, but may be possibly transformed via a `f`-to-`x`
    transformation (see :py:class:`HistogramAnalysis`).

    The fit model is exactly the same as :math:`y(x)` given by
    :py:func:`fit_fn_a2()`, except that it is parametrized in a different way in
    the hope that the fit optimization is more stable.  Namely, the fit function
    is rewritten as a function of :math:`(a, x_0, y_0, m)` as:

    .. math::

         y(x) = - a (x - x_0)^2 + y_0 + z_m(x / x_0)\ .

    .. versionadded:: 5.2

    """
    return -a*np.square(x - x0) + y0 + zm(np.divide(x,x0), m)


QuantumErrorBars = collections.namedtuple('QuantumErrorBars', ('f0', 'Delta', 'gamma', 'y0') )
"""
A named tuple to store the quantum error bars.
"""

QuantumErrorBarsX = collections.namedtuple('QuantumErrorBarsX', ('x0', 'Delta', 'gamma', 'y0') )
"""
A named tuple to store the quantum error bars, but where `x0` is used
instead of `f0`, i.e., the figure of merit is not transformed according to the
`xtof` transformation specified to the :py:class:`HistogramAnalysis`
constructor.

This class should only be used internally.

.. versionadded:: 5.2
"""

def fit_histogram(normalized_histogram, fit_fn, ftox, **kwopts):
    """
    Fit the histogram data to a model function.

    You should prefer to use the higher-level :py:class:`HistogramAnalysis`
    class directly, which calls this function internally and automatically.
    Only call this function if you are implementing very specific functionality
    and you know what you are doing.

    :param normalized_histogram: the final histogram data, provided as a
        :py:class:`tomographer.HistogramWithErrorBars` class (or a subclass
        thereof), and normalized to a proper probability density (see
        :py:meth:`tomographer.HistogramWithErrorBars.normalized()`).

    :param ftox: a function which transforms the figure of merit values (`f`-values) into
        the natural parameter for the fit function (`x`-values).  For example, for the
        fidelity, one should use :math:`x = 1 - f`.

    :param threshold_fraction: a fraction (i.e. value between 0 and 1) of the maximum peak
        value over which the data points are kept; values below the threshold are ignored
        for the fit. (By default, `threshold_fraction=0` and all points are considered;
        since the error bars are taken into account this should be fine in most cases.)

    :param kwopts: additional keyword options are passed on to
        `scipy.optimize.curve_fit(...)`.
    """

    f = normalized_histogram.values_center
    x = ftox(f)
    p = normalized_histogram.bins
    errp = normalized_histogram.delta

    #print(f)
    #print(p)
    #print(errp)

    # Optionally: filter and keep only the points which are above a threshold_fraction of
    # the max of the prob curve.  Indeed, low points are usually not very useful.
    #
    # Points are not filted out by default (threshold_fraction=0)
    thresfrac = kwopts.pop('threshold_fraction', 0)
    thres = thresfrac*np.max(p)
    #display(thres)

    # get the indexes in the histogram where the points are selected (above threshold)
    idxok = np.where(p>thres)

    # form data series of those points we keep
    fok = f[idxok]
    xok = x[idxok]
    logpok = np.log(p[idxok])
    errlogpok = np.divide(errp[idxok], p[idxok]);

    # and perform the fit
    popt, pcov = sciopt.curve_fit(fit_fn, xok, logpok, sigma=errlogpok, absolute_sigma=True, **kwopts)
    #display(popt)
    #display(pcov)
    
    # return value: an object with useful values stored as properties
    d = _Ns()
    d.f = f
    d.x = x
    d.p = p
    d.errp = errp
    d.normalized_histogram = normalized_histogram
    d.idxok = idxok
    d.fok = fok
    d.xok = xok
    d.logpok = logpok
    d.errlogpok = errlogpok
    # fit data
    d.popt = popt
    d.pcov = pcov

    return d



def deskew_logmu_curve(a2, a1, m, c):
    """
    .. deprecated:: 5.2
       Use :py:class:`FitParamToQuErrorBars_a2` instead.

    De-skew the fit model to obtain a second order approximation at the peak maximum (see
    details of how to calculate the quantum error bars in our paper).

    Usage::

        (a, x0, y0) = deskew_logmu_curve(a2, a1, m, c)
    """
    
    x0 = (np.sqrt(np.square(a1) + 8*a2*m) - a1) / (4*a2)
    y0 = -a2*np.square(x0) - a1*x0 + m*np.log(x0) + c
    a = a2 + m / (2 * np.square(x0))

    return (a, x0, y0)


def reskew_logmu_curve(m, a, x0, y0):
    """
    .. deprecated:: 5.2
       Use :py:class:`FitParamToQuErrorBars_a2` instead.
    """
    a2 = a - m / (2 * np.square(x0))
    a1 = (a2*m - 2*np.square(a2)*np.square(x0)) / (a2*x0)
    c = y0 + a2*np.square(x0) + a1*x0 - m*np.log(x0)
    return (a2, a1, m, c)


def qu_error_bars_from_deskewed(xtof, m, a, x0, y0):
    """
    .. deprecated:: 5.2
       Use :py:class:`FitParamToQuErrorBars_a2` instead.
    """
    f0 = xtof(x0)
    Delta = 1/np.sqrt(a)
    gamma = m / (6 * np.square(a) * np.power(x0,3))
    return (f0, Delta, gamma)


def qu_error_bars_to_deskewed_c(ftox, f0, Delta, gamma, y0=1):
    """
    .. deprecated:: 5.2
       Use :py:class:`FitParamToQuErrorBars_a2` instead.
    """
    a = 1.0 / np.square(Delta)
    x0 = ftox(f0)
    m = gamma * 6 * np.square(a) * np.power(x0,3)
    return (m, a, x0, y0)



class FitParamToQuErrorBarsBase(object):
    """
    Base class for converters between fit parameters for a specific fit model,
    and (standard) quantum error bars.

    The members of this base class do not do anything.  This class should be
    overridden to provide functionality.  See for instance
    :py:class:`FitParamToQuErrorBars_a2` and
    :py:class:`FitParamToQuErrorBars_direct`.

    .. versionadded:: 5.2
    """

    def fitParamBounds(self):
        """
        Return the bounds the fit parameters should be constrained to by default.
        (In a format suitable for `scipy.curve_fit`, see also
        `HistogramAnalysis` constructor.)
        """
        return None

    def guessFitParamsFromQuErrorBarsX(self, qu_err_bars):
        """
        Return a tuple of fit parameters corresponding to the given quantum error
        bars `qu_err_bars`, given as a :py:class:`QuantumErrorBarsX` instance
        (i.e., parameters `x0`, `Delta`, `gamma`, along with the offset `y0`).
        """
        return None

    def calcQuantumErrorBarsX(self, fit_params):
        """
        Return the quantum error bars (as a :py:class`QuantumErrorBarsX` instance,
        i.e., specifying the parameters `x0`, `Delta`, `gamma` and `y0`)
        corresponding to the given fit parameters `fit_params`.
        """
        return QuantumErrorBarsX(None, None, None, None)


class FitParamToQuErrorBars_a2(FitParamToQuErrorBarsBase):
    """
    Quantum error bars calculator for fit parameters resulting from fitting to
    the fit model function :py:func:`fit_fn_a2()`.

    .. versionadded:: 5.2
    """

    # params: (a2, a1, m, c)

    def fitParamBounds(self):
        return ( (0, -np.inf, 0, -np.inf), np.inf, )

    def fitParamsFromQuErrorBarsX(self, x0, Delta, gamma, y0):
        
        a = 1.0 / np.square(Delta)
        m = gamma * 6 * np.square(a) * np.power(x0,3)

        a2 = a - m / (2 * np.square(x0))
        a1 = (a2*m - 2*np.square(a2)*np.square(x0)) / (a2*x0)
        c = y0 + a2*np.square(x0) + a1*x0 - m*np.log(x0)

        return (a2, a1, m, c)

    def guessFitParamsFromQuErrorBarsX(self, q, educated_guess=True):

        if not educated_guess:
            return fitParamsFromQuErrorBarsX(*q)

        (x0, Delta, gamma, y0) = q

        # we might have to try with different gamma's
        for niter in range(10):

            (a2, a1, m, c) = self.fitParamsFromQuErrorBarsX(x0, Delta, gamma, y0)

            logger.debug(("FitParamToQuErrorBars_a2.fitParamsFromQuErrorBarsX(): "
                          "x0={},Delta={},gamma={} -> a2={},a1={},m={},c={}")
                         .format(q.x0,q.Delta,gamma, a2,a1,m,c))

            if a2 > 0 and m > 0:
                # all set
                return (a2,a1,m,c)

            # try with different (smaller) gamma's, in case we guessed gamma incorrectly
            gamma /= 2

        logger.info("Having trouble guessing fit parameters graphically, resorting to generic guess")
        return (500.0, 100.0, 20., 0)


    def calcQuantumErrorBarsX(self, p):
        (a2, a1, m, c) = p
        if a2 < 0:
            raise ValueError("Invalid value of a2: {} <= 0".format(a2))
        if a2 < 1e-6:
            # if a2 == 0:
            x0 = m / a1
        else:
            x0 = (np.sqrt(np.square(a1) + 8*a2*m) - a1) / (4*a2)
        y0 = -a2*np.square(x0) - a1*x0 + m*np.log(x0) + c
        a = a2 + m / (2 * np.square(x0))
        Delta = 1/np.sqrt(a)
        gamma = m / (6 * np.square(a) * np.power(x0,3))
        return QuantumErrorBarsX(x0, Delta, gamma, y0)


class FitParamToQuErrorBars_direct(FitParamToQuErrorBarsBase):
    """
    Quantum error bars calculator for fit parameters resulting from fitting to
    the fit model function :py:func:`fit_fn_direct()`.

    .. versionadded:: 5.2
    """

    # params: (a, x0, y0, m)

    def fitParamBounds(self):
        return ( (0, 0, -np.inf, 0), np.inf, )

    def guessFitParamsFromQuErrorBarsX(self, q):
        a = 1/np.square(q.Delta)
        return ( a,
                 q.x0,
                 q.y0,
                 q.gamma * (6 * np.square(a) * np.power(q.x0,3)) )

    def calcQuantumErrorBarsX(self, p):
        (a, x0, y0, m) = p
        a2 = a - m/(2*x0*x0)
        if a2 < 0:
            logger.warning(
                "Fit parameters: invalid value for a={}; corresponding a2={} should be >= 0"
                .format(a, a2))

        return QuantumErrorBarsX(
            x0= x0,
            Delta= 1/np.sqrt(a),
            gamma= m / (6 * np.square(a) * np.power(x0,3)),
            y0= y0
        )

        
FitModelSpec = collections.namedtuple('FitModelSpec', ('fn', 'converter',))
"""
Type used to specify a fit model function along with a "converter"
implementation.

See :py:data:`fit_models`.

.. versionadded:: 5.2
"""


fit_models = {
    'a2': FitModelSpec(fit_fn_a2, FitParamToQuErrorBars_a2()),
    'direct': FitModelSpec(fit_fn_direct, FitParamToQuErrorBars_direct()),
}
"""
Dictionary of known, built-in fit models.  The key is the the built-in name
(e.g., 'a2'), and the value is a :py:class:`FitModelSpec` named tuple type with
information about the function to use as fit model as well as a converter
implementation allowing to convert the fit parameters to quantum error bars.

.. versionadded:: 5.2
"""


def calc_redchi2(x, y, dy, theo_fn, num_fit_params):

    redchi2 = np.sum( [
        np.square( ( y[k] - theo_fn(x[k]) ) / dy[k] )
        for k in range(len(x))
    ])
    redchi2 /= (len(x) - num_fit_params)

    return redchi2


def guess_querrorbarsx_from_data(histogram, ftox_fn):
    # read off graphically x0, y0, Delta, and gamma from the data -- just a rough guess
    # with absolutely no garantees
    
    idxmax = np.argmax(histogram.bins)

    fvals = histogram.values_center
    xvals = np.array([ ftox_fn(f) for f in fvals ])
    pvals = histogram.bins

    x0 = xvals[idxmax]
    f0 = fvals[idxmax]
    expy0 = histogram.bins[idxmax]
    y0 = np.log(expy0)

    #logger.debug("x0={},f0={},expy0={},y0={}".format(x0,f0,expy0,y0))

    # find values above 1/e relative height of y0
    thresval = expy0*np.exp(-1.0)

    # linearly interpolate left intersect from bordering data points
    firstidxabove = np.argmax(histogram.bins > thresval)
    if firstidxabove == 0: firstidxabove = 1
    x0m = ( xvals[firstidxabove-1] +
            (thresval-histogram.bins[firstidxabove-1])/(histogram.bins[firstidxabove]-histogram.bins[firstidxabove-1])
            *(xvals[firstidxabove]-xvals[firstidxabove-1]) )

    lastidxabove = histogram.numBins() - np.argmax(histogram.bins[::-1] > thresval) - 1
    if lastidxabove+1 >= histogram.numBins(): lastidxabove -= 1
    x0p = ( xvals[lastidxabove+1] +
            (thresval-histogram.bins[lastidxabove+1])/(histogram.bins[lastidxabove]-histogram.bins[lastidxabove+1])
            *(xvals[lastidxabove]-xvals[lastidxabove+1]) )
    
    Delta = np.absolute((x0p - x0m) / 2.0)
    gamma = 2*np.absolute((x0p + x0m) / 2.0 - x0)

    #logger.debug("xvals={}".format(xvals))

    logger.debug(("guess_querrorbarsx_from_data(): "
                  "firstidxabove={},lastidxabove={},x0m={},x0p={},x0={},f0={},Delta={},gamma={}")
                 .format(firstidxabove,lastidxabove,x0m,x0p,x0,f0,Delta,gamma))

    return QuantumErrorBarsX(x0=x0, Delta=Delta, gamma=gamma, y0=y0)





class HistogramAnalysis(object):
    """
    Take care of analyzing histogram data obtained from performing a random walk over
    state space according to the distribution :math:`\\mu_{B^n}(\\cdot)` defined by the
    tomography data, while collecting a histogram of a figure of merit.

    Arguments to the constructor:

      - `final_histogram`: the final histogram returned by the random walks
        procedure.  It is expected to be a
        `tomographer.HistogramWithErrorBars`.

      - `ftox`: Specify how to transform the figure of merit value `f` into
        the `x` coordinate for fit. Specify the transformation as a pair of
        values `(h, s)` in the relation `x = s*(f-h)` or `f=s*x+h`, where h
        can be any constant, and where `s` must be plus or minus one. By
        default there is no transformation (`x=f`, corresponding to
        `ftox=(0,1)`.  For the fidelity, you should use `x = 1-f`
        (`ftox=(1,-1)`). For an entanglement witness, you might use `x = 2-f`
        (`ftox=(2,-1)`) for example.

      - `fit_fn`: a function which serves as fit model (for the log of the
        data).  Specify None (the default fit model), the strings 'a2' or
        'direct', or a custom callable to use as fit model.  The 'a2' model (the
        default) is the "straightforward" one, using :py:func:`fit_fn_a2()`,
        while the 'direct' model is aimed at better fit optimization stability,
        using :py:func:`fit_fn_direct()`.  If a custom callable is used, the
        quantum error bars cannot be automatically calculated and a call to
        :py:meth:`quantumErrorBars()` will fail.

        .. note:: for the 'direct' fit model, it is currently not possible to
           enforce the condition that :math:`a - m/(2 x_0^2) \geq 0` (required
           from :math:`a_2 \geq 0`), so this condition must be checked manually.
           A warning will be issued if you call `quantumErrorBars()` in this
           case.

        .. versionchanged:: 5.2
           Support for additional built-in fit models.

      - additional named arguments in `kwopts` are passed on to
        :py:func:`fit_histogram`.
    """
    def __init__(self, final_histogram, ftox=(0,1), fit_fn=None, **kwopts):
        self.final_histogram = final_histogram
        self.normalized_histogram = final_histogram.normalized()
        if ftox[1] not in [1, -1]:
            raise ValueError("Invalid value of `s` in `ftox=(h,s)`: s=%r"%(ftox[1]))
        self.ftox_hs = ftox

        if callable(fit_fn):
            # User specified custom callable fit function
            self.custom_fit_fn = True
            self.fit_fn = fit_fn
            self.fit_fn_name = "<custom fit function>"
        else:
            self.custom_fit_fn = False

            if fit_fn is None:
                fit_fn = 'a2'

            if fit_fn not in fit_models:
                raise ValueError("Invalid fit model name: {}".format(fit_fn))

            self.fit_fn_name = fit_fn
            self.fit_fn = fit_models[fit_fn].fn
            self.fit_converter = fit_models[fit_fn].converter

            if 'bounds' not in kwopts:
                # a2, a1, m, c
                kwopts['bounds'] = self.fit_converter.fitParamBounds()

            if 'p0' not in kwopts:
                qxguess = guess_querrorbarsx_from_data(self.normalized_histogram, self.ftox)
                kwopts['p0'] = self.fit_converter.guessFitParamsFromQuErrorBarsX(qxguess)
                logger.debug("Guessing initial fit params = {!r}".format(kwopts['p0']))

        self.FitParamsType = collections.namedtuple('FitParamsType', inspect.getargspec(self.fit_fn).args[1:])

        self.fit_histogram_result = fit_histogram(self.normalized_histogram, fit_fn=self.fit_fn,
                                                  ftox=self.ftox, **kwopts)
        self.fit_params = self.FitParamsType(*self.fit_histogram_result.popt)
        self.fit_params_cov = self.FitParamsType(*np.diagonal(self.fit_histogram_result.pcov))

        self.printFitParameters(print_func=logger.debug,show_cov=True)

        # calculate reduced chi2 statistic
        xok = self.fit_histogram_result.xok
        logpok = self.fit_histogram_result.logpok
        errlogpok = self.fit_histogram_result.errlogpok

        self.redchi2 = calc_redchi2(xok, logpok, errlogpok,
                                    lambda x: self.fit_fn(x, *self.fit_params), num_fit_params=4)
        
        redchi2_warn_threshold = kwopts.get('redchi2_warn_threshold', 2.0)

        if self.redchi2 > redchi2_warn_threshold :
            logger.warning(("Reduced chi-squared statistic = {:.4g}. It could be that the fit "
                            "model isn't good or that the fit optimization failed.").format(self.redchi2))

    
    def xtof(self, x):
        """
        Convert an `x`-value (natural variable of the fit model) to the value of the figure of
        merit (`f`).
        """
        return self.ftox_hs[1]*x + self.ftox_hs[0]

    def ftox(self, f):
        """
        Convert an `f`-value (value of the figure of merit) to an `x`-value (natural variable
        of the fit model).
        """
        return self.ftox_hs[1]*(f - self.ftox_hs[0])

    def fitParameters(self):
        """
        Return the parameters of the fit.  The return value type is a named tuple whose fields
        are the argument names of the fit model `fit_fn` specified to the constructor.
        For example, for the default fit model :py:func:`fit_fn_a2`, the fields are
        `(a2, a1, m, c)`.
        """
        return self.fit_params

    def fitRedChi2(self):
        """
        Return the `Reduced chi-squraed statistic
        <https://en.wikipedia.org/wiki/Reduced_chi-squared_statistic>`_ for the fit.
        Should be close to `1` for a good fit.
        """
        return self.redchi2

    def printFitParameters(self, print_func=print, show_cov=False):
        """
        Display the fit parameters.

        By default the values are displayed using `print(...)`. You may specify a custom
        print function if you want them displayed somewhere else.

        The fit parameters are returned in the same format as :py:meth:`fitParameters()`.
        """
        def fmt_param_val(x, cov):
            if show_cov:
                return '{:>8g}  +-  {:.3g}'.format(x, np.sqrt(cov))
            else:
                return '{:g}'.format(x)
        print_func("Fit parameters:\n" + "\n".join([
            '{:>12s} = {}'.format(k, fmt_param_val(getattr(self.fit_params, k), getattr(self.fit_params_cov, k)))
            for k in self.fit_params._fields
        ]))
        return self.fit_params

    def quantumErrorBars(self):
        """
        Calculate the quantum error bars.

        This function is only available if you use the default fit model or one
        of the predefined fit models (i.e., if you didn't specify a custom
        callable to the `fit_fn` argument in the constructor).

        Returns a :py:class:`QuantumErrorBars` named tuple.
        """
        
        if self.custom_fit_fn:
            raise RuntimeError("You cannot call quantumErrorBars() when you specify a custom fit model.")

        qx = self.fit_converter.calcQuantumErrorBarsX(self.fit_params)

        return QuantumErrorBars(self.xtof(qx.x0), qx.Delta, qx.gamma, qx.y0)


    def printQuantumErrorBars(self, print_func=print):
        """
        Calculates and displays the values of the quantum error bars.

        The quantum error bars are returned, provided in the same format as
        :py:meth:`quantumErrorBars()`.
        """
        q = self.quantumErrorBars()
        print_func(("Quantum Error Bars:\n"+
                    "          f0 = {f0:.4g}\n"+
                    "       Delta = {Delta:.4g}\n"+
                    "       gamma = {gamma:.4g}\n").format(
            f0=q.f0,
            Delta=q.Delta,
            gamma=q.gamma))
        return q


    def plot(self,
             log_scale=False,
             xlabel='Distribution of values',
             plot_deskewed_gaussian=True,
             show_plot=True,
             curve_npts=200,
             **kwopts):
        """
        Plot the histogram data using `matplotlib`.

        :param log_scale: if `True`, then use a logarithmic scale for the y-axis.

        :param xlabel: the label for the x axis (i.e., the figure of merit). Note this
            is the `f`-value, not the `x`-value.

        :param show_plot: if `True` (the default), then the plot is displayed immediately.
            If `False`, then the returned object has an additional method `show()` which can
            be used to display the plot.
        
        :param curve_npts: Number of points for the smooth fit curve plot.

        The return value is an object with the attributes `fig` (the `matplotlib` figure
        object) and `ax` (the `matplotlib` axes object) which you can use to set specific
        custom properties.  If you specified `show_plot=False`, then you should call the
        `show()` method on the returned object in order to show the plots (which is simply
        an alias for `matplotlib.pyplot.show()`.
        """
        
        import matplotlib.pyplot as plt

        d = _Ns()

        d.fig = plt.figure()

        d.ax = d.fig.add_subplot(111)

        d.ax.set_xlabel(xlabel)
        d.ax.set_ylabel('probability density')

        if log_scale:
            d.ax.set_yscale('log')

        f = self.fit_histogram_result.f
        p = self.fit_histogram_result.p
        errp = self.fit_histogram_result.errp

        fvals = np.linspace(np.min(f), np.max(f), curve_npts)

        d.ax.errorbar(x=f, y=p, yerr=errp, c='b', fmt='.', label='numerics')
        d.ax.plot(fvals, np.exp(self.fit_fn(self.ftox(fvals), *self.fit_params)), c='r', label='fit')
        if plot_deskewed_gaussian:
            if self.custom_fit_fn:
                raise RuntimeError("Cannot plot deskewed gaussian with a custom fit; "
                                   "please use plot_deskewed_gaussian=False.")
            qf = self.quantumErrorBars()
            logging.debug("using quantum error bars = {!r}".format(qf))
            d.ax.plot(fvals, np.exp( -np.square(self.ftox(fvals)-self.ftox(qf.f0))/np.square(qf.Delta) + qf.y0 ),
                      c='g', label='deskewed Gaussian')

        if show_plot:
            plt.show()
        else:
            d.show = plt.show

        return d;
        
#


def load_tomorun_csv_histogram_file(fn):
    """
    Load a histogram data file produced by executing the `tomorun` executable
    program (instead of via the :py:mod:`tomographer.tomorun` python module).

    The argument `fn` is the file name. It is expected to be a TAB-separated CSV
    file where the first row (header) is blindly discarded.  Any fourth column
    is disregarded.

    .. note:: This function blindly assumes, without checking, that the first
              column is a list of linearly spaced values (which is what the
              `tomorun` executable always produces currently).

    The returned value is a :py:class:`tomographer.HistogramWithErrorBars`
    object.

    .. versionadded:: 5.0
    """
    import tomographer
    
    dat = np.loadtxt(fn, skiprows=1)
    # dat[:,0] should be lin-spaced values corresponding to the left edges of the histogram bins
    fmin = dat[0,0]
    numbins = dat.shape[0]
    binresolution = dat[1,0]-dat[0,0]
    fmax = dat[numbins-1,0] + binresolution
    h = tomographer.HistogramWithErrorBars(fmin, fmax, numbins)
    h.load(dat[:,1], dat[:,2])
    return h
