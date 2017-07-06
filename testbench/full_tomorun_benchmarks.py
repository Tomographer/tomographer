
import itertools
import os.path
import collections
import datetime
import psutil
from pipes import quote as shellquote

from pkg_resources import parse_version

from tomographerbuild import TomographerBuild, timeit_call


settings_matrix = {
    'asserts': {
        True: '-g -UNDEBUG',
        False: '-DNDEBUG',
    },
    'optim': {
        'O3': '-O3',
#        'Ofast': '-Ofast',
    },
    'target_arch': [
        'auto',
#        'core',
    ],
    'multiproc': [
        'openmp',
        'cxxthreads',
    ],
    'tomorun_static': {
        'default': [],
        'fix-dim': ['-DTOMORUN_CUSTOM_FIXED_DIM=4',
                    '-DTOMORUN_CUSTOM_FIXED_MAX_DIM=4',
                    '-DTOMORUN_CUSTOM_MAX_POVM_EFFECTS=36'],
        'fix-log': ['-DTOMORUN_MAX_LOG_LEVEL=INFO'],
        'float': ['-DTOMORUN_REAL=float'],
    },
    'version': [
        'v1.0',
        'v4.0',
        'v5.0',
        'v5.1',
    ],
    'compiler': {
#        'gcc-6': ('gcc-6', 'g++-6'),
        'clang-4.0': ('clang-4.0', 'clang++-4.0'),
    },
}


SettingValue = collections.namedtuple('SettingValue', ('valname','value',))


def get_settings_list(settings_matrix):
    """
    For instance, if::

        settings_matrix={ 'asserts': {True: '', False: '-DNDEBUG'},
                          'optimlevel': ['-O3', '-Ofast'] }

    Then, return the list of all possible combinations of settings, ie.

        settings_list = [
          {'asserts': (True, ''),
           'optimlevel': ('-O3', '-O3'), },
          {'asserts': (True, ''),
           'optimlevel': ('-Ofast', '-Ofast'), },
          {'asserts': (False, '-DNDEBUG'),
           'optimlevel': ('-O3', '-O3'), },
          {'asserts': (False, '-DNDEBUG'),
           'optimlevel': ('-Ofast', '-Ofast'), },
        ]

    """

    def itervals(vals_dict_or_list):
        if isinstance(vals_dict_or_list, dict):
            return vals_dict_or_list.items()
        # else, just a list; duplicate each item as two-tuple of value-name, value
        return ( (v,v) for v in vals_dict_or_list )

    each_setting_as_list = [
        [(k,SettingValue(valname=valname,value=val)) for valname, val in itervals(vals)]
        for k,vals in settings_matrix.items()
    ]

    return [dict(x) for x in itertools.product(*each_setting_as_list)]



def get_tomographer_build(settings):
    
    cc, cxx = settings['compiler'].value

    rawversion = settings['version'].value[1:]  # e.g., "4.0"

    if ( settings['multiproc'].valname == 'cxxthreads'
         and parse_version(rawversion) < parse_version('4.0') ):
        # no support for C++11 threads in Tomographer < 4.0
        return None

    if ( len(settings['tomorun_static'].value)
         and parse_version(rawversion) < parse_version('2.0') ):
        # TOMORUN_XXXX config macros introduced in 2.0 only
        return None

    cxx_flags = []
    if 'gcc' in settings['compiler'].value[0]:
        total_ram_gb = psutil.virtual_memory().total/(2.0**30)
        if total_ram_gb < 4.0:
            cxx_flags += '--param ggc-min-expand=4 --param ggc-min-heapsize=8192'.split()
        elif total_ram_gb < 8.0:
            cxx_flags += '--param ggc-min-expand=10 --param ggc-min-heapsize=32768'.split()

    cmakeopts = [
        '-DCMAKE_BUILD_TYPE=Release',
        '-DCMAKE_C_COMPILER='+cc,
        '-DCMAKE_CXX_COMPILER='+cxx,
        '-DTOMORUN_MULTIPROC='+settings['multiproc'].value,
        '-DTARGET_ARCHITECTURE='+settings['target_arch'].value,
        '-DCMAKE_CXX_FLAGS_RELEASE={} {}'.format(settings['optim'].value, settings['asserts'].value),
        # Don't use -DTOMORUN_CXX_FLAGS so that this works for earlier versions of tomographer
        '-DCMAKE_CXX_FLAGS=-Wall -Wextra {} {}'.format(
            " ".join([shellquote(f) for f in settings['tomorun_static'].value]),
            " ".join([shellquote(f) for f in cxx_flags])),
    ]

    build = TomographerBuild(settings['version'].value, cmakeopts)

    if parse_version(rawversion) < parse_version('3.0a0'):
        build.tomorun_exe = os.path.abspath(
            os.path.join(build.work_tomographer_build, 'cxx', 'tomorun', 'tomorun')
        )
    else:
        build.tomorun_exe = os.path.abspath(
            os.path.join(build.work_tomographer_build, 'tomorun', 'tomorun')
        )

    return build


#
# Run the settings matrix and get benchmarks
#

settings_list = get_settings_list(settings_matrix)

# build everything first
builds = [
    (settings, get_tomographer_build(settings))
    for settings in settings_list
]

fresults = open('_full_bench_results/res_{}.txt'.format(datetime.datetime.now().strftime("%Y%m%d-%H%M%S")), 'w')

fresults.write("# Running benchmarks -- "+datetime.datetime.now().isoformat(' ')+"\n\n")
fresults.write("import collections\n\n")
fresults.write("SettingValue = collections.namedtuple('SettingValue', ('valname','value',))\n\n")
fresults.write("\nsettings_matrix = "+repr(settings_matrix)+"\nsettings_list="+repr(settings_list)+"\n\n")

fresults.write("results = [ None ] * {}\n\n".format(len(builds)))

for n in range(len(builds)):

    settings, build = builds[n]

    if build is None:
        fresults.write("# build #{} is disabled\n\n".format(n))
        continue

    no_ctrl_options = ['--no-control-step-size', '--no-control-binning-converged',]
    if parse_version(settings['version'].value[1:]) < parse_version('5.0a1'):
        # options --no-control* introduced in Tomographer 5.0
        no_ctrl_options = []

    res = timeit_call(
        [
            build.tomorun_exe,
            '--config',
            'tomorun-config-EntglWitness',
            '--nice=0',
        ] + no_ctrl_options,
        cwd='examples/two-qubits-Bell',
        repeat=5
    )

    fresults.write("results[{}] = {}\n\n".format(n, repr(res)))

fresults.close()

