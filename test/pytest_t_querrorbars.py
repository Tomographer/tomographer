
import numpy.testing as npt

import tomographer.querrorbars

import unittest

class Stub(unittest.TestCase):

    def test_stub(self):
        self.assertAlmostEqual(tomographer.querrorbars.fit_fn_default(1.0, 2.0, 3.0, 4.0, 12.0), 7.0)
#


class TestQuErrBarsTransf(unittest.TestCase):

    def test_stuff(self):

        fitparams = [ 250.1, 100.0, 42.1, 8.3 ] # a2, a1, m, c
        desk = tomographer.querrorbars.deskew_logmu_curve(*fitparams) # deskewed (a, x0, y0)
        m = fitparams[2]
        npt.assert_almost_equal( tomographer.querrorbars.reskew_logmu_curve(m, *desk),
                                 fitparams )

        def xtof(x):
            return 1 - x
        def ftox(f):
            return 1 - f

        qeb = tomographer.querrorbars.qu_error_bars_from_deskewed(xtof, m, *desk)
        npt.assert_almost_equal( tomographer.querrorbars.qu_error_bars_to_deskewed_c(ftox, *qeb, y0=desk[2]),
                                 [m] + list(desk) )


# normally, this is not needed as we are being run via pyruntest.py, but it might be
# useful if we want to run individually picked tests
if __name__ == '__main__':
    unittest.main()
