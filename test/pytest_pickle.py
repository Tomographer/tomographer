#!/usr/bin/env python

from __future__ import print_function

import sys
import re
import os
import numpy as np
import numpy.testing as npt
try:
    import cPickle as pickle
except ImportError:
    import pickle

import logging
logging.basicConfig(level=logging.NOTSET) # test LONGDEBUG logging level

import unittest

# import the module
import tomographer


pyversion = sys.version_info[0]

pickledatadirroot = os.path.join(os.path.dirname(os.path.realpath(os.path.abspath(__file__))),
                                 '_pickledata-py{}'.format(pyversion))

class PickleVersionsLoad(unittest.TestCase):
    
    def test_load_pickled(self):

        for version in os.listdir(pickledatadirroot):
            pickledatadir = os.path.join(pickledatadirroot, version)
            
            print("\nTESTING VERSION {}\n\n".format(version))

            for fn in os.listdir(pickledatadir):

                with open(os.path.join(pickledatadir, fn), 'rb') as f:
                    try:
                        obj = pickle.load(f)
                    except Exception:
                        print("ERROR: Failed to load {}.".format(os.path.join(pickledatadir,fn)))
                        raise

                print("Loaded object:\n",repr(obj))



# normally, this is not needed as we are being run via pyruntest.py, but it might be
# useful if we want to run individually picked tests
if __name__ == '__main__':
    unittest.main()
