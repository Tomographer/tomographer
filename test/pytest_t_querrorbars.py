
import tomographer.querrorbars

import unittest

class Stub(unittest.TestCase):

    def test_stub(self):
        self.assertAlmostEqual(tomographer.querrorbars.fit_fn_default(1.0, 2.0, 3.0, 4.0, 12.0), 7.0)
