
from __future__ import print_function

import os
import os.path
import subprocess

from tomographerbuild import TomographerBuild, timeit_call

from tomorun import build_v1_0_optim_release, build_v4_1_optim_release, \
    build_g287a5ae_optim_release, build_g287a5ae_GCC6_optim_release





# timeit_call(
#     [
#         os.path.abspath(os.path.join(build_v1_0_optim_release.work_tomographer_build,
#                                      'cxx', 'tomorun', 'tomorun')),
#         '--config',
#         'tomorun-config-EntglWitness'
#     ],
#     cwd='examples/two-qubits-Bell')


# timeit_call(
#     [
#         os.path.abspath(os.path.join(build_v4_1_optim_release.work_tomographer_build,
#                                      'tomorun', 'tomorun')),
#         '--config',
#         'tomorun-config-EntglWitness'
#     ],
#     cwd='examples/two-qubits-Bell')


# timeit_call(
#     [
#         os.path.abspath(os.path.join(build_g287a5ae_optim_release.work_tomographer_build,
#                                      'tomorun', 'tomorun')),
#         '--config',
#         'tomorun-config-EntglWitness',
#         '--no-control-step-size',
#         '--no-control-binning-converged',
#     ],
#     cwd='examples/two-qubits-Bell')

# timeit_call(
#     [
#         os.path.abspath(os.path.join(build_g287a5ae_GCC6_optim_release.work_tomographer_build,
#                                      'tomorun', 'tomorun')),
#         '--config',
#         'tomorun-config-EntglWitness',
#         '--no-control-step-size',
#         '--no-control-binning-converged',
#     ],
#     cwd='examples/two-qubits-Bell')

timeit_call(
    [
        os.path.abspath(os.path.join(build_g287a5ae_GCC6_optim_release.work_tomographer_build,
                                     'tomorun', 'tomorun')),
        '--config',
        'tomorun-config-EntglWitness',
        '--no-control-step-size',
        '--no-control-binning-converged',
#        '--no-binning-analysis',
    ],
    cwd='examples/two-qubits-Bell')