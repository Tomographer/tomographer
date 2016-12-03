#!/usr/bin/env python

import re

import logging
logging.basicConfig(level=logging.DEBUG)

import unittest

# import the module
import tomographer


class BasicStuff(unittest.TestCase):
    def test_version(self):
        # test for __version__
        print("Tomographer version: "+tomographer.__version__)
        # parse __version__
        m = re.match(r'^v(?P<maj>\d+)\.(?P<min>\d+)(?P<suffix>[a-zA-Z][a-zA-Z0-9]*)?'
                     r'(-(?P<gitsuffix>\d+-?g?[a-fA-F0-9]*))?$',
                     tomographer.__version__)
        self.assertIsNotNone(m)
        print("Verision MAJ=", m.group('maj'), " MIN=", m.group('min'), " SUFFIX=", m.group('suffix'),
              " GITSUFFIX=", m.group('gitsuffix'))

    def test_cxxlogger(self):
        # test that the cxxlogger object exists, and check we can set a log level.
        tomographer.cxxlogger.level = logging.INFO


