
from ipywidgets import IntProgress, HTML, VBox, HBox
from IPython.display import display

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
    SimpleProgressBar = _SimpleProgressBar_ConsoleImpl



class RandWalkProgressBar(SimpleProgressBar):
    def __init__(self, label_msg_html="Random Walk Progress: "):
        super(RandWalkProgressBar, self).__init__(label_msg_html=label_msg_html)

    def progress(self, *args, **kwargs):
        raise RuntimeError("Set as callback RandWalkProgressBar.progress_fn, not RandWalkProgressBar.progress")

    def progress_fn(self, report):
        super(RandWalkProgressBar, self).progress(
            fraction_done=report.total_fraction_done,
            addinfo=report.getHumanReport()
            )

    def displayFinalInfo(self, report_str):
        if self.addinfo:
            self.addinfo.value += "<pre style=\"display: block\">{}</pre>".format(escapehtml(report_str))
        else:
            print(report_str)



