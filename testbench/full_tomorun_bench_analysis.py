
import re
import os.path
import itertools

import numpy as np # np.prod


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

def merge_results(*dlist):

    dmerged = Store(settings_list=[], results=[], settings_matrix={})

    for d in dlist:
        dmerged.settings_list += d.settings_list
        dmerged.results += d.results
        for k,v in d.settings_matrix.items():
            if k not in dmerged.settings_matrix:
                dmerged.settings_matrix[k] = {}
            if isinstance(v, list):
                dmerged.settings_matrix[k].update(dict([(vl,vl) for vl in v]))
            else:
                dmerged.settings_matrix[k].update(v)

    return dmerged


default_lbl_formatters = {
    'version': str,
    'tomorun_static': str,
    'asserts': lambda v: '+a' if v else '-a',
    'multiproc': lambda v: {'openmp':'OMP','cxxthreads':'C++','sequential':'seq'}.get(v, v),
    'compiler': lambda v: os.path.basename(v),
    'optim': lambda v: v,
    'target_arch': lambda v: 'arch:{}'.format(v),
}
class _ValFormatterWithKey(object):
    def __init__(self, key):
        self.key = key
    def __call__(self, value):
        return "{}={}".format(self.key, value)

class _MkLabel(object):
    def __init__(self, lbl_dict, fld_order):
        self.lbl_dict = lbl_dict
        self.fld_order = fld_order

    def __call__(self, sett, valaccess=lambda x: x.valname):
        return " ".join([ self.lbl_dict.get(f,_ValFormatterWithKey(f))(valaccess(sett[f]))
                          for f in self.fld_order ])

def subset_results(filt_dict, d, fld_order, lbl_dict={}):
    idx = [i for i, x in enumerate(d.settings_list)
           if ( x is not None and d.results[i] is not None and
                all([x[k].valname == v for k,v in filt_dict.items()]) )
           ]

    idx = sorted(idx, key=lambda x: tuple([d.settings_list[x][f] for f in fld_order]))

    lbl_dict_ok = dict(default_lbl_formatters)
    lbl_dict_ok.update(lbl_dict)

    #def _mklabel(i):
    #    #return " ".join([ "{}={}".format(k,d.settings_list[i][k].valname) for k in d.settings_list[i]])
    #    return " ".join([ lbl_dict_ok.get(f,_ValFormatterWithKey(f))(d.settings_list[i][f].valname) for f in fld_order if f not in filt_dict ])

    mklabel = _MkLabel(lbl_dict_ok, [ f for f in fld_order if f not in filt_dict ])

    return Store(d=d, filt_dict=filt_dict, didx=idx, n=len(idx),
                 fld_order=fld_order,
                 sett=[d.settings_list[i] for i in idx],
                 resbest=[d.results[i]['best'] for i in idx],
                 res=[d.results[i] for i in idx],
                 settinglabels=[ mklabel(d.settings_list[i]) for i in idx ],
                 lbl_dict=lbl_dict_ok,
                 )


def simple_subset_plot(subd, spacings=[1,1],
                       colorsettings=['tomorun_static','asserts'],
                       colorfn='jet'):

    import matplotlib.patches as mpatches
    import matplotlib.pyplot as plt
    
    fig = plt.figure()
    ax = fig.add_subplot(111)
    fig.subplots_adjust(left=0.04, right=0.96, bottom=0.3)

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

    class colorfn_cmap(object):
        def __init__(self, cmap):
            import matplotlib.colors
            import matplotlib.cm
            num_colors = np.prod([len(subd.d.settings_matrix[colorsetting])
                                  for colorsetting in colorsettings])
            cNorm = matplotlib.colors.Normalize(vmin=0, vmax=num_colors-1)
            scalarMap = matplotlib.cm.ScalarMappable(norm=cNorm, cmap=cmap)

            #print([k for k in subd.d.settings_matrix['tomorun_static']])

            self.color_settings_list = list(itertools.product(
                *[ [ (sfld, svl) for svl in subd.d.settings_matrix[sfld] ]
                  for sfld in colorsettings ]
                ))
            #print("color_settings_list=", self.color_settings_list)

            assert num_colors == len(self.color_settings_list)

            self.settings_to_colors = [
                (dict(self.color_settings_list[k]), scalarMap.to_rgba(k),)
                for k in range(num_colors)
                ]
        
            #print('settings_to_colors=', self.settings_to_colors)

        def __call__(self, d):
            #print("d=", d)
            return [ c for valset, c in self.settings_to_colors
                     if valset == d ][0]

    if not callable(colorfn):
        colorfn = colorfn_cmap(colorfn)

    mklabelx = _MkLabel(subd.lbl_dict, [f for f in subd.fld_order if f not in subd.filt_dict and f not in colorsettings])
    mklabelcolor = _MkLabel(subd.lbl_dict, colorsettings)

    barcolors = [ ]
    legends_for_sett = {}
    for j in range(len(subd.sett)):
        
        if subd.resbest[j] is None:
            continue
    
        settdic = dict([ (k,v.valname)
                           for k,v in subd.sett[j].items() if k in colorsettings ])
        thiscolor = colorfn(settdic)
        barcolors.append( thiscolor )

        settdicfs = frozenset(settdic.items())
        if settdicfs not in legends_for_sett:
            legends_for_sett[settdicfs] = mpatches.Patch(color=thiscolor, label=mklabelcolor(settdic, valaccess=lambda x: x))
            

    #print('barcolors=',barcolors)

    settinglabels = [ mklabelx( dict([(k,v) for k,v in subd.sett[j].items()]) )
                      for j in range(len(subd.sett)) ]

    #print("Legend..ary:\n", list(zip(* legends_for_sett.items() )), sep='')

    ax.legend(handles=list(legends_for_sett.values()))

    ax.bar(xpos, subd.resbest, align='center', color=barcolors, alpha=0.8)
    ax.set_xticks(xpos)
    ax.set_xticklabels(settinglabels, rotation=90)
    for item in ax.get_xticklabels():
        item.set_fontsize(8)

    ax.set_xlim(-sum(spacings), max(xpos)+sum(spacings))

    plt.show()
    


if __name__ == '__main__':

    import argparse

    parser = argparse.ArgumentParser(os.path.basename(__file__))
    parser.add_argument('bench_files', nargs='+')

    args = parser.parse_args()

    ds = []
    for fn in args.bench_files:
        ds.append(load_results(fn))

    d = merge_results(*ds)

    subd = subset_results(
        {'optim':'O3', 'target_arch':'auto',},
        d,
        ['version','compiler','tomorun_static','asserts','multiproc']
    )

    simple_subset_plot(subd, spacings=[0,1,1])
