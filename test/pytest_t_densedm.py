#!/usr/bin/env python

from __future__ import print_function

import re
import numpy as np
import numpy.testing as npt

import logging
logging.basicConfig(level=logging.NOTSET)#DEBUG)

import unittest

# import the module
import tomographer.densedm
import tomographer


class tDMTypes(unittest.TestCase):
    def test_basic(self):
        dmt = tomographer.densedm.DMTypes(4) # two qubits
        self.assertEqual(dmt.dim, 4)
        self.assertEqual(dmt.dim2, 4*4)
        self.assertEqual(dmt.ndof, 4*4-1)

        # constructor with named argument
        dmt = tomographer.densedm.DMTypes(dim=4)

class tParamX(unittest.TestCase):
    def test_basic(self):
        # constructor
        p = tomographer.densedm.ParamX(tomographer.densedm.DMTypes(4))
        # constructor, with named argument
        p = tomographer.densedm.ParamX(dmt=tomographer.densedm.DMTypes(4))
        
        A = np.array([
            [1, 5-11j, 6-12j,  8-14j],
            [5+11j, 2, 7-13j,  9-15j],
            [6+12j, 7+13j, 3, 10-16j],
            [8+14j, 9+15j, 10+16j, 4],
        ])
        x = np.array([1, 2, 3, 4] + [ np.sqrt(2)*k for k in range(5, 17) ])

        # can convert from Hermitian matrix to X param
        npt.assert_array_almost_equal(p.HermToX(A), x)
        # also with keyword argument
        npt.assert_array_almost_equal(p.HermToX(Herm=A), x)

        # can convert back to Hermitian matrix from X param
        npt.assert_array_almost_equal(p.XToHerm(x), A)
        # also with keyword argument
        npt.assert_array_almost_equal(p.XToHerm(x=x), A)
                                      

class tIndepMeasLLH(unittest.TestCase):
    def test_basic(self):
        dmt = tomographer.densedm.DMTypes(dim=2) # qubit

        # constructor
        llh = tomographer.densedm.IndepMeasLLH(dmt)

        # constructor with named argument
        llh = tomographer.densedm.IndepMeasLLH(dmt=dmt)

        # .dmt attribute
        self.assertEqual(llh.dmt.dim, dmt.dim)

        # .numEffects attribute
        self.assertEqual(llh.numEffects, 0)

        # .addMeasEffect()
        llh.addMeasEffect(np.array([1, 0, 0, 0]), 15)
        self.assertEqual(llh.numEffects, 1)
        npt.assert_array_almost_equal(llh.Exn(), np.array([[1, 0, 0, 0]]))
        npt.assert_array_almost_equal(llh.Exn(0), np.array([1, 0, 0, 0]))
        npt.assert_array_equal(llh.Nx(), np.array([15]))
        self.assertEqual(llh.Nx(0), 15)

        def dotest(llh):
            self.assertEqual(llh.numEffects, 2)
            npt.assert_array_almost_equal(llh.Exn(), np.array([[1, 0, 0, 0],
                                                                [0, 1, 0, 0]]))
            npt.assert_array_almost_equal(llh.Exn(0), np.array([1, 0, 0, 0]))
            npt.assert_array_almost_equal(llh.Exn(1), np.array([0, 1, 0, 0]))
            npt.assert_array_equal(llh.Nx(), np.array([15, 85]))
            self.assertEqual(llh.Nx(0), 15)
            self.assertEqual(llh.Nx(1), 85)

        llh.addMeasEffect(np.array([[0, 0],
                                    [0, 1]]), 85)
        dotest(llh)

        # .resetMeas()
        llh.resetMeas()
        self.assertEqual(llh.numEffects, 0)
        
        # ensure that .addMeasEffect() doens't accept 1-D vectors with imaginary values 
        with self.assertRaises(Exception):
            llh.addMeasEffect(np.array([1, 0, 0.5j, 0]), 10)

        # .setMeas()
        llh.setMeas([ np.array([[1, 0], [0, 0]]) , np.array([[0, 0], [0, 1]]) ], [15, 85])
        dotest(llh)
        llh.setMeas(np.array([ [[1, 0], [0, 0]], [[0, 0], [0, 1]] ]), np.array([15, 85]))
        dotest(llh) # in particular, check that setMeas() cleared the earlier measurement data
        llh.setMeas([ np.array([1, 0, 0, 0]), np.array([0, 1, 0, 0]) ], np.array([15, 85]))
        dotest(llh)
        llh.setMeas(np.array([ [1, 0, 0, 0], [0, 1, 0, 0] ]), np.array([15, 85]))
        dotest(llh)

        # ensure that .setMeas() doens't accept 1-D vectors with imaginary values 
        with self.assertRaises(Exception):
            llh.setMeas([ np.array([1, 0, 0.5j, 0]) ], [ 10 ])

        # and check that we can calculate the log-likelihood function
        llh.setMeas(np.array([ [1, 0, 0, 0], [0, 1, 0, 0] ]), np.array([15, 85]))
        llhval = llh.logLikelihoodX(np.array([0.5, 0.5, -0.1, 0]))
        self.assertAlmostEqual(llhval, 15*np.log(0.5)+85*np.log(0.5))
        llhval = llh.logLikelihoodRho(np.array([[0.4, 0.1j],
                                                [-0.1j, 0.6]]))
        self.assertAlmostEqual(llhval, 15*np.log(0.4)+85*np.log(0.6))

        # ensure that .logLikelihoodX() doens't accept 1-D vectors with imaginary values
        #
        # ### With PyBind11, a warning is generated instead.
        #
        #with self.assertRaises(Exception):
        #    llh.logLikelihoodX(np.array([1, 0, 0.5j, 0]))

        try:
            import cPickle as pickle
        except ImportError:
            import pickle

        ss = pickle.dumps(llh,2)
        llh2 = pickle.loads(ss)
        llhval2 = llh2.logLikelihoodRho(np.array([[0.4, 0.1j],
                                                  [-0.1j, 0.6]]))
        self.assertAlmostEqual(llhval2, 15*np.log(0.4)+85*np.log(0.6))


# normally, this is not needed as we are being run via pyruntest.py, but it might be
# useful if we want to run individually picked tests
if __name__ == '__main__':
    unittest.main()
