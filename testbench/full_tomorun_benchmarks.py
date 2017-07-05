
import itertools
import os.path
import datetime

from pkg_resources import parse_version

import tomographerbuild


cmake_release_opts = [
    '-DCMAKE_BUILD_TYPE=Release',
    '-DTARGET_ARCHITECTURE=auto',
    '-DCMAKE_CXX_FLAGS_RELEASE=-O3 -g -UNDEBUG',
]

settings_matrix = {
    'asserts': {
        True: '-g -UNDEBUG',
#        False: '-DNDEBUG',
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
#        'cxxthreads',
    ],
    'tomorun_static': {
        'default': '',
#        'fix-dim': '-DTOMORUN_CUSTOM_FIXED_DIM=4;-DTOMORUN_CUSTOM_FIXED_MAX_DIM=4;-DTOMORUN_CUSTOM_MAX_POVM_EFFECTS=36',
#        'fix-log': '-DTOMORUN_MAX_LOG_LEVEL=INFO',
#        'float': '-DTOMORUN_REAL=float',
    },
    'version': [
#        'v1.0',
#        'v4.0',
#        'v5.0',
        'v5.1',
    ],
    'compiler': {
        'gcc-6': ('gcc-6', 'g++-6'),
#        'clang-4.0': ('clang-4.0', 'clang++-4.0'),
    },
}



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
        [(k,(valname,val)) for valname, val in itervals(vals)]
        for k,vals in settings_matrix.items()
    ]

    return [dict(x) for x in itertools.product(*each_setting_as_list)]



def get_tomographer_build(settings):
    
    cc, cxx = settings['compiler'][1]

    rawversion = settings['version'][1][1:]  # e.g., "4.0"

    if ( parse_version(rawversion) < parse_version('4.0')
         and settings['multiproc'][0]=='cxxthreads' ):
        # no support for C++11 threads in Tomographer < 4.0
        return None

    cmakeopts = [
        '-DCMAKE_BUILD_TYPE=Release',
        '-DCMAKE_C_COMPILER='+cc,
        '-DCMAKE_CXX_COMPILER='+cxx,
        '-DTOMORUN_MULTIPROC='+settings['multiproc'],
        '-DTARGET_ARCHITECTURE='+settings['target_arch'],
        '-DCMAKE_CXX_FLAGS_RELEASE={} {}'.format(settings['optim'], settings['asserts']),
        # Don't use -DTOMORUN_CXX_FLAGS so that this works for earlier versions of tomographer
        '-DCMAKE_CXX_FLAGS=-Wall -Wextra '+settings['tomorun_static'],
    ]

    build = TomographerBuild(settings['version'], cmakeopts)

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

fresults = open('_full_bench_results/res_{}.txt'.format(datetime.datetime.now().strftime("%Y%m%d-%H%M%S")), 'a')

fresults.write("# Running benchmarks -- "+datetime.datetime.now().isoformat(' ')+"\n")
fresults.write("\nsettings_matrix = "+repr(settings_matrix)+"\nsettings_list="+repr(settings_list)+"\n\n")

fresults.write("results = [ None ] * "+str(len(builds))+"\n\n")

results = [ None ] * len(builds)

for n in range(len(builds)):

    settings, build = builds[n]

    if build is None:
        fresults.write("# build #{} is disabled\n\n".format(n))
        continue

    no_ctrl_options = ['--no-control-step-size', '--no-control-binning-converged',]
    if parse_version(settings['version'][1][1:]) < parse_version('5.0a1'):
        # options --no-control* introduced in Tomographer 5.0
        no_ctrl_options = []

    res = timeit_call(
        [
            build.tomorun_exe,
            '--config',
            'tomorun-config-EntglWitness',
        ] + no_ctrl_options,
        cwd='examples/two-qubits-Bell'
    )

    fresults.write("results[{}] = {}\n\n".format(n, repr(res)))

fresults.close()

