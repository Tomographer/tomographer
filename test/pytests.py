#!/usr/bin/env python

import sys
import os
import os.path

print("Run: "+" ".join(sys.argv))

TomographerPyMod = sys.argv[1]
sys.path.append(os.path.dirname(TomographerPyMod))

# import the module
import tomographer

print("Tomographer version: "+tomographer.__version__)







print("\n\nNOT YET IMPLEMENTED: TODO: WRITE TESTS FOR THE TOMOGRAPHER PYTHON INTERFACE\n")


# it's ugly on travis if the whole test suite fails because of unwritten tests...
# only report failure when running the tests locally
if 'TRAVIS_OS' not in os.environ:
    sys.exit(1)
