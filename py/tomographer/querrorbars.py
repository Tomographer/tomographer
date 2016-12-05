
import collections # namedtuple
import inspect
import numpy as np
import scipy
import scipy.optimize as sciopt
import matplotlib.pyplot as matplt

class _Ns:
    pass


def fit_fn_default(x, a2, a1, m, c):
    return -a2*np.square(x) - a1*x + m*np.log(x) + c

FitParameters = collections.namedtuple('FitParameters', ('a2', 'a1', 'm', 'c') )
QuantumErrorBars = collections.namedtuple('QuantumErrorBars', ('f0', 'Delta', 'gamma') )


def fit_histogram(final_histogram, fit_fn, ftox, **kwopts):

    f = final_histogram.values_center
    x = ftox(f)
    p = final_histogram.bins
    errp = final_histogram.delta

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
    d.final_histogram = final_histogram
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
    (a, x0, y0) = deskew_logmu_curve(a2, a1, m, c)
    de-skew the fit model to obtain a second order approximation.
    """
    
    x0 = (np.sqrt(a1**2 + 8*a2*m) - a1) / (4*a2)
    y0 = -a2*x0**2 - a1*x0 + m*np.log(x0) + c
    a = a2 + m / (2 * x0**2)

    return (a, x0, y0)


def qu_error_bars_from_deskewed(xtof, m, a, x0, y0):
    f0 = xtof(x0)
    Delta = 1/np.sqrt(a)
    gamma = m / (6 * a**2 * x0**3)
    return (f0, Delta, gamma)


class HistogramAnalysis(object):
    def __init__(self, final_histogram, ftox=(0,1), fit_fn=None, **kwopts):
        """
        Arguments:
          - final_histogram: the final histogram returned by the random walks
            procedure.  It is expected to be a
            `tomographer.UniformBinsHistogramWithErrorBars`.
          - ftox: Specify how to transform the figure of merit value `f` into
            the `x` coordinate for fit. Specify the transformation as a pair of
            values `(h, s)` in the relation `x = s*(f-h)` or `f=s*x+h`, where h
            can be any constant, and where `s` must be plus or minus one. By
            default there is no transformation (`x=f`, corresponding to
            `ftox=(0,1)`.  For the fidelity, you should use `x = 1-f`
            (`ftox=(1,-1)`). For an entanglement witness, you might use `x = 2-f`
            (`ftox=(2,-1)`) for example.
          - fit_fn: a function which serves as fit model (for the log of the data)
        """
        self.final_histogram = final_histogram
        if ftox[1] not in [1, -1]:
            raise ValueError("Invalid value of `s` in `ftox=(h,s)`: s=%r"%(ftox[1]))
        self.ftox_hs = ftox

        if fit_fn:
            # User specified custom fit function
            self.custom_fit_fn = True
            self.fit_fn = fit_fn
        else:
            self.custom_fit_fn = False
            self.fit_fn = fit_fn_default
            if 'bounds' not in kwopts:
                # a2, a1, m, c
                kwopts['bounds'] = ( (0, -np.inf, 0.1, -np.inf), np.inf, )
            if 'p0' not in kwopts:
                kwopts['p0'] = [ 1000, 300, 10, 0 ]

        self.FitParamsType = collections.namedtuple('FitParamsType', inspect.getargspec(self.fit_fn).args[1:])

        self.fit_histogram_result = fit_histogram(final_histogram, fit_fn=self.fit_fn, ftox=self.ftox, **kwopts)
        self.fit_params = self.FitParamsType(*self.fit_histogram_result.popt)
    
    def xtof(self, x):
        return self.ftox_hs[1]*x + self.ftox_hs[0]

    def ftox(self, f):
        return self.ftox_hs[1]*(f - self.ftox_hs[0])

    def fitParameters(self):
        return self.fit_params

    def printFitParameters(self, print_func=print):
        print_func("Fit parameters:\n" + "\n".join([
            '{:10s} = {:g}'.format(k, getattr(self.fit_params, k))
            for k in self.fit_params._fields
        ]))
#        print_func("Fit Parameters:\n\ta2 = {a2:.4g}\n\ta1 = {a1:.4g}\n\t m = {m:.4g}\n\t c = {c:.4g}\n".format(
#            a2=self.fit_params.a2,
#            a1=self.fit_params.a1,
#            m=self.fit_params.m,
#            c=self.fit_params.c))
        return self.fit_params

    def quantumErrorBars(self):
        if self.custom_fit_fn:
            raise RuntimeError("You cannot call quantumErrorBars() when you specify a custom fit model.")

        (f0, Delta, gamma) = qu_error_bars_from_deskewed(
            self.xtof,
            self.fit_params.m,
            *deskew_logmu_curve(*self.fit_params)
        )
        return QuantumErrorBars(f0, Delta, gamma)

    def printQuantumErrorBars(self, print_func=print):
        q = self.quantumErrorBars()
        print_func(("Quantum Error Bars:\n"+
                    "           f0 = {f0:.4g}\n"+
                    "        Delta = {Delta:.4g}\n"+
                    "        gamma = {gamma:.4g}\n").format(
            f0=q.f0,
            Delta=q.Delta,
            gamma=q.gamma))
        return q


    def plot(self, log_scale=False, xlabel='Distribution of values', **kwopts):
        
        d = _Ns()

        d.fig = matplt.figure()

        d.ax = d.fig.add_subplot(111)

        d.ax.set_xlabel(xlabel)
        d.ax.set_ylabel('probability density')

        if log_scale:
            d.ax.set_yscale('log')

        f = self.fit_histogram_result.f
        p = self.fit_histogram_result.p
        errp = self.fit_histogram_result.errp

        d.ax.errorbar(x=f, y=p, yerr=errp, c='b', fmt='.', label='numerics')
        d.ax.plot(np.linspace(np.min(f), np.max(f), 100),
                  np.exp(self.fit_fn(self.ftox(np.linspace(np.min(f), np.max(f), 100)),
                                     *self.fit_params)),
                  c='r', label='fit')

        if kwopts.get('show_plot', True):
            matplt.show()
        else:
            d.show = matplt.show

        return d;
        
