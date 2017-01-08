#!/usr/bin/env python

from __future__ import print_function

import re
import numpy as np
import numpy.testing as npt

import logging
logging.basicConfig(level=logging.DEBUG)

import unittest

# import the module
import tomographer.tools.densedm
import tomographer.tools.densedm.mle
import tomographer
import tomographer.densedm


class SimulateMeasurements(unittest.TestCase):

    def test_sim(self):

        rho_sim = np.array([[0.9, 0], [0, 0.1]])
        Mk = tomographer.tools.densedm.PauliMeasEffectsQubit

        n = 1000

        d = tomographer.tools.densedm.simulate_measurements(rho_sim, Mk, n)

        self.assertEqual(d.Nm[0] + d.Nm[1], n)
        self.assertEqual(d.Nm[2] + d.Nm[3], n)
        self.assertEqual(d.Nm[4] + d.Nm[5], n)

        # Hoeffding's inequality: Prob( |N(+) - p*n| > eps*n ) < 2*exp(-2*eps^2*n)
        #
        # --> so the probability to deviate by more than 0.1 fraction is bounded by
        #     2*exp(-2 * 0.1**2 * n) ~ 4e-9   (for n=1000)
        
        self.assertLessEqual( (d.Nm[0] - 0.5*n) , 0.1*n )
        self.assertLessEqual( (d.Nm[2] - 0.5*n) , 0.1*n )
        self.assertLessEqual( (d.Nm[4] - 0.9*n) , 0.1*n )


class Mle(unittest.TestCase):
    
    def test_mle(self):
        
        Emn = sum(tomographer.tools.densedm.PauliMeasEffectsQubit, [])
        Nm = np.array([250, 250, 250, 250, 500, 0]) # really extreme example
        
        llh = tomographer.densedm.IndepMeasLLH(tomographer.densedm.DMTypes(2))
        llh.setMeas(Emn, Nm)

        (rho_MLE, d) = tomographer.tools.densedm.mle.find_mle(llh)

        # we know the exact solution, rho_MLE = |0><0|

        npt.assert_array_almost_equal(rho_MLE,
                                      np.array([[1, 0], [0, 0]]))



# normally, this is not needed as we are being run via pyruntest.py, but it might be
# useful if we want to run individually picked tests
if __name__ == '__main__':
    unittest.main()
