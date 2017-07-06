
import re
import os.path


class Store(object):
    def __init__(self, **kwargs):
        super(Store, self).__init__()
        self.__dict__.update(kwargs)

    def _store_items(self):
        return [(k, v) for k, v in self.__dict__.items() if k[:1] != '_']

    def __repr__(self):
        return 'Store({})'.format([','.join(['{}={}'.format(k,repr(v)) for k,v in self._store_items()])])

    def __str__(self):
        return ', '.join(['{}={}'.format(k, repr(v)) for k,v in self._store_items()])

# Ugly hack to be able to load our results saved as python code
settings_list = None
results = None
settings_matrix = None

def load_results(res_file_name):
    
    with open(res_file_name) as f:
        exec('global settings_list, results, settings_matrix\n\n'+f.read(), globals(), locals())

    return Store(settings_list=settings_list, results=results, settings_matrix=settings_matrix)



default_lbl_formatters = {
    'version': str,
    'tomorun_static': str,
    'asserts': lambda v: '+a' if v else '-a',
    'multiproc': lambda v: {'openmp':'OMP','cxxthreads':'C++','sequential':'seq'}.get(v, v),
    'compiler': lambda v: os.path.basename(v[0]),
    'optim': lambda v: v,
    'target_arch': lambda v: 'arch:{}'.format(v),
}
class _ValFormatterWithKey(object):
    def __init__(self, key):
        self.key = key
    def __call__(self, value):
        return "{}={}".format(self.key, value)


def subset_results(filt_dict, d, fld_order, lbl_dict={}):
    idx = [i for i, x in enumerate(d.settings_list)
           if ( x is not None and d.results[i] is not None and
                all([x[k].valname == v for k,v in filt_dict.items()]) )
           ]

    idx = sorted(idx, key=lambda x: tuple([d.settings_list[x][f] for f in fld_order]))

    lbl_dict_ok = dict(default_lbl_formatters)
    lbl_dict_ok.update(lbl_dict)

    def mklabel(i):
        #return " ".join([ "{}={}".format(k,d.settings_list[i][k].valname) for k in d.settings_list[i]])
        return " ".join([ lbl_dict_ok.get(f,_ValFormatterWithKey(f))(d.settings_list[i][f].valname) for f in fld_order if f not in filt_dict ])

    return Store(d=d, didx=idx, n=len(idx),
                 fld_order=fld_order,
                 sett=[d.settings_list[i] for i in idx],
                 resbest=[d.results[i]['best'] for i in idx],
                 res=[d.results[i] for i in idx],
                 settinglabels=[ mklabel(i) for i in idx ]
                 )


def simple_subset_plot(subd, spacings=[1,1]):

    import matplotlib.pyplot as plt
    
    fig = plt.figure()
    ax = fig.add_subplot(111)
    fig.subplots_adjust(bottom=0.3)

    # pad with zeros up to length of fld_order
    spacings = spacings + [0]*(len(subd.fld_order) - len(spacings))

    xpos = []
    curpos = 0
    for i in range(len(subd.sett)):
        if i == 0:
            xpos.append(curpos)
            curpos += 1
            continue
        for fi in range(len(subd.fld_order)):
            f = subd.fld_order[fi]
            if subd.sett[i][f].valname != subd.sett[i-1][f].valname:
                curpos += spacings[fi]
        xpos.append(curpos)
        curpos += 1

    ax.bar(xpos, subd.resbest, align='center', color=[0.8,0.2,0], alpha=0.8)
    ax.set_xticks(xpos)
    ax.set_xticklabels(subd.settinglabels, rotation=90)

    plt.show()
    
