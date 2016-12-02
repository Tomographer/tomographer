
from ipywidgets import IntProgress, HTML, VBox, HBox
from IPython.display import display

try:
    import html
    escapehtml = html.escape
except ImportError:
    import cgi
    escapehtml = cgi.escape


# give ourselves a nice progress bar
class SimpleProgressBar(object):
    def __init__(self, label_msg_html="Progress: ", displaybox=True):
        super(SimpleProgressBar, self).__init__()
        self.progress_widget = IntProgress(min=0, max=100, value=0)
        self.label = HTML("<span style=\"padding-right: 1em;\">"+label_msg_html+"</span>")
        self.labelval = HTML()
        if displaybox:
            self.box = HBox(children=[self.label, self.progress_widget, self.labelval])
            display(self.box)
    def __enter__(self):
        self.progress_widget.value = 0
        self.progress_widget.bar_style = 'info'
        return self
    def progress(self, **kwargs):
        if 'fraction_done' in kwargs:
            pcvalue = int(kwargs['fraction_done']*100)
        elif 'percent_done' in kwargs:
            pcvalue = int(kwargs['percent_done'])
        else:
            raise ArgumentError("No fraction_done nor percent_done argument given")
        self.progress_widget.value = pcvalue
        self.labelval.value = "<span style=\"font-weight: bold; padding-left: 1em;\">{}% done</span>".format(pcvalue)
    def __exit__(self, etype, evalue, etraceback):
        if etype:
            self.progress_widget.bar_style = 'danger'
            self.labelval.value = "<span style=\"font-weight: bold; padding-left: 1em;\">(interrupted)</span>"
        else:
            self.progress_widget.value = 100
            self.progress_widget.bar_style = 'success'
            self.labelval.value = "<span style=\"font-weight: bold; padding-left: 1em;\">finished</span>"


class RandWalkProgressBar(SimpleProgressBar):
    def __init__(self, label_msg_html="Random Walk Progress: "):
        super(RandWalkProgressBar, self).__init__(displaybox=False)
        self.innerhbox = HBox(children=[self.label, self.progress_widget, self.labelval])
        self.rwinfo = HTML()
        self.vbox = VBox(children=[self.innerhbox, self.rwinfo])
        display(self.vbox)

    def progress(self, *args, **kwargs):
        raise RuntimeError("Set as callback RandWalkProgressBar.progress_fn, not RandWalkProgressBar.progress")

    def progress_fn(self, report):
        super(RandWalkProgressBar, self).progress(fraction_done=report.total_fraction_done)
        self.rwinfo.value = "<pre style=\"display: block\">{}</pre>".format(escapehtml(report.getHumanReport()))

    def displayFinalInfo(self, report_str):
        self.rwinfo.value += "<pre style=\"display: block\">{}</pre>".format(escapehtml(report_str))
