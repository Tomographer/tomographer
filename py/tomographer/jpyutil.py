
"""
Utilities for Jupyter notebooks.
"""

try:
    from ipywidgets import IntProgress, HTML, VBox, HBox
    from IPython.display import display
except ImportError:
    pass # no problem, not in Jupyter/ipython notebook

try:
    import html
    escapehtml = html.escape
except ImportError:
    import cgi
    escapehtml = cgi.escape

class _Ns: pass

def in_ipynb():
    try:
        # if this is accessible, assume we're in IPython/Jupyter
        cfg = get_ipython().config
        return True
    except NameError:
        return False


class _SimpleProgressBar_JupyterImpl(object):
    def __init__(self, label_msg_html="Progress: ", displaybox=True):
        super(_SimpleProgressBar_JupyterImpl, self).__init__()
        self.progress_widget = IntProgress(min=0, max=100, value=0)
        self.label = HTML("<span style=\"padding-right: 1em;\">"+label_msg_html+"</span>")
        self.labelval = HTML()
        self.innerhbox = HBox(children=[self.label, self.progress_widget, self.labelval])
        self.addinfo = HTML()
        self.vbox = VBox(children=[self.innerhbox, self.addinfo])
        if displaybox:
            display(self.vbox)

    def __enter__(self):
        self.progress_widget.value = 0
        self.progress_widget.bar_style = 'info'
        return self

    def progress(self, fraction_done, addinfo=None):
        pcvalue = int(fraction_done*100)
        self.progress_widget.value = pcvalue
        self.labelval.value = "<span style=\"font-weight: bold; padding-left: 1em;\">{}% done</span>".format(pcvalue)
        self.addinfo.value = ("<pre style=\"display: block\">{}</pre>".format(escapehtml(addinfo)) if addinfo else "")

    def __exit__(self, etype, evalue, etraceback):
        if etype:
            self.progress_widget.bar_style = 'danger'
            self.labelval.value = "<span style=\"font-weight: bold; padding-left: 1em;\">(interrupted)</span>"
        else:
            self.progress_widget.value = 100
            self.progress_widget.bar_style = 'success'
            self.labelval.value = "<span style=\"font-weight: bold; padding-left: 1em;\">finished</span>"

class _SimpleProgressBar_ConsoleImpl(object):
    def __init__(self, label_msg_html="Progress: ", displaybox=True):
        super(_SimpleProgressBar_ConsoleImpl, self).__init__()
        self.addinfo = None

    def __enter__(self):
        return self

    def progress(self, fraction_done, addinfo=None):
        totw = 80 - 8;
        w = int(fraction_done*totw)
        print("|{}{}| {:5.2%}".format('*'*w, ' '*(totw-w), fraction_done))
        if addinfo:
            print(addinfo)

    def __exit__(self, etype, evalue, etraceback):
        pass


if in_ipynb():
    SimpleProgressBar = _SimpleProgressBar_JupyterImpl
else:
    # This also happens when parsed by Sphinx
    SimpleProgressBar = _SimpleProgressBar_ConsoleImpl
SimpleProgressBar.__name__ = 'SimpleProgressBar'

#
# Document the SimpleProgressBar API here, so it is visible in either case.
#
SimpleProgressBar.__doc__ = """
A simple progress bar implementation, to be used in a Python `with` statement::

    with SimpleProgressBar("Working very hard ... ") as prg:
        for i in range(10000):
            # do stuff that takes time
            ...
            # update the progress bar
            prg.progress(fraction_done=i/10000.0)

This class works both inside a Jupyter notebook, providing a nice graphical progress bar,
as well as in any other context such as a simple python console script, in which case
information is simply displayed in the terminal using `print(...)`.

In addition, it is possible to display additional information to simply the fraction done.
You can do this by specifying the argument `add_info` to the :py:meth:`progress()`
callback.  Inside a Jupyter notebook, the information is displayed below the progress bar
and is constantly updated at each call of :py:meth:`progress()`.  In the console version,
the information is simply displayed each time using `print(...)`.

Note: When we are inside a Jupyter notebook, there is an additional attribute which is
exposed called `addinfo`.  It is an `IPython.display.HTML` widget which serves to display
the additional info. By testing for this attribute you can display custom stuff in there,
e.g. a final report after the work has finished.

"""
SimpleProgressBar.progress.__doc__ = """
progress(fraction_done[, add_info=None])

The callback function to update the progress bar (or to display new information on the
console).

:param fraction_done: is the fraction of the job done, a real number between zero and one.
:param add_info: any additional information (formatted as plain text) which should be
    displayed alongside the progress bar. If `None` (or not specified), then no additional
    information is displayed.

"""







class RandWalkProgressBar(SimpleProgressBar):
    """
    A progress bar suitable for displaying the status of random walk tasks (as for
    :py:func:`tomographer.tomorun.tomorun()`, for example).

    This class automatically provides a callback function :py:meth:`progress_fn` which
    accepts a :py:class:`tomographer.multiproc.FullStatusReport`, and is capable of
    displaying meaningful additional information alongside the progress bar.  The callback
    :py:meth:`progress_fn` is suitable to be specified directly to, e.g.,
    :py:func:`tomographer.tomorun.tomorun()`.

    Example::

        result = None
        with RandWalkProgressBar() as prg:
            result = tomographer.tomorun.tomorun(..., progress_fn=prg.progress_fn)
            prg.displayFinalInfo(result['final_report_runs'])

    This class works both inside a Jupyter notebook as well as in other contexts such as a
    console for a simple Python script.

    .. note:: you should not use the `progress()` callback on this object, as you will not
        take advantage of the ability to display the information contained in
        :py:class:`~tomographer.multiproc.FullStatusReport`.
    """
    def __init__(self, label_msg_html="Random Walk Progress: "):
        super(RandWalkProgressBar, self).__init__(label_msg_html=label_msg_html)

    def progress(self, *args, **kwargs):
        """
        Do not call this method. It will raise a `RuntimeError`.
        """
        raise RuntimeError("Set as callback RandWalkProgressBar.progress_fn, not RandWalkProgressBar.progress")

    def progress_fn(self, report):
        """
        Update the progress bar and additional info to display the current status of the
        random walk specified in `report`, provided as a
        :py:class:`~tomographer.multiproc.FullStatusReport` object.

        This method is suitable to be specified as a callback to, e.g.,
        :py:func:`tomographer.tomorun.tomorun()`.
        """
        super(RandWalkProgressBar, self).progress(
            fraction_done=report.total_fraction_done,
            addinfo=report.getHumanReport()
            )

    def displayFinalInfo(self, report_str):
        """
        Display some text in the "additional information" area of the progress bar.

        This is useful if you want to display some final report after the random walks
        have completed.

        This method works both within a Jupyter notebook as well as in a console.
        """
        if self.addinfo:
            self.addinfo.value += "<pre style=\"display: block\">{}</pre>".format(escapehtml(report_str))
        else:
            print(report_str)



