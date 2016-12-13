
import tomographer.querrorbars

import unittest

class Stub(unittest.TestCase):

    def test_stub(self):
        self.assertAlmostEqual(tomographer.querrorbars.fit_fn_default(1.0, 2.0, 3.0, 4.0, 12.0), 7.0)
#





# normally, this is not needed as we are being run via pyruntest.py, but it might be
# useful if we want to run individually picked tests
if __name__ == '__main__':
    unittest.main()
