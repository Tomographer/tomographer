
import sys
import os
import os.path

import argparse

parser = argparse.ArgumentParser()
parser.add_argument('--tomographer-cxx-lib', help='Location of the compiled _tomographer_cxx library.')
parser.add_argument('--tomographer-source', help='Location of the tomographer sources (root directory).')
parser.add_argument('--mode', action='store', dest='mode', default='cmake-built')
parser.add_argument('--test', help='Name of test to run (python module name in test/ subdirectory).')

print("\npyruntest: \n" + "\n".join( ["\t"+x for x in sys.argv] ) + "\n\n")

(args, rest_argv) = parser.parse_known_args()

TomographerPyMod = args.tomographer_cxx_lib
TomographerSourcePath = args.tomographer_source

morepaths = []
if args.mode == 'cmake-built':
    morepaths = [
        os.path.dirname(TomographerPyMod),
        os.path.join(TomographerSourcePath, 'py'),
        os.path.join(TomographerSourcePath, 'test')
        ]
elif args.mode == 'setup-py-built':
    if TOMOGRAPHER_PYTHONPATH in os.environ:
        morepaths = [ os.path.abspath(os.environ.get('TOMOGRAPHER_PYTHONPATH')) ]
else:
    raise ValueError("Unknown python test run mode: %s"%(args.mode))


# add locations to sys.path -- before all system paths in order to override any installed
# tomographer package
sys.path = morepaths + sys.path

print(sys.path)


#
# finally, run the test we want
#
import importlib
testmodule = importlib.import_module(args.test)


import unittest
unittest.main(module=testmodule, argv=[ sys.argv[0] ] + rest_argv)
