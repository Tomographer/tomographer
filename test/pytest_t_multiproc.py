#!/usr/bin/env python

from __future__ import print_function

import re
import numpy as np
import numpy.testing as npt

import logging
logging.basicConfig(level=logging.DEBUG)

import unittest

# import the module
import tomographer.multiproc
import tomographer


#
# currently there are only the status report classes, well, those aren't the most crucial
# part of the program
#

class MultiProcStuff(unittest.TestCase):
    pass




# normally, this is not needed as we are being run via pyruntest.py, but it might be
# useful if we want to run individually picked tests
if __name__ == '__main__':
    unittest.main()
