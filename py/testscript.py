#!/usr/bin/env python

from __future__ import print_function

import tomographer
import numpy as np




print("1:")
tomographer.test_eigen(np.array([1, 2, 3, 4]))

print("2:")
tomographer.test_eigen(np.array([[1.0,2.0],[3, 1e-5]]))

print("and then:")

print(tomographer.test_eigen2(np.array([[1.0,2.0],[3, 1e-5]])))
print(tomographer.test_eigen2(np.array([[1.0j,2.0j],[3, 1e-5]])))

print(tomographer.testgetmatrix_d(np.array([[1.0,2.0],[3, 1e-5]])))
print(tomographer.testgetmatrix_cd(np.array([[1.0,2.0],[3, 1e-5]])))
print(tomographer.testgetmatrix_cd(np.array([[1.0j,2.0j],[3, 1e-5]])))

print(tomographer.testgetmatrix_i(np.array([[1,2],[3, 1]])))

try:
    tomographer.test_eigen(np.array([1, 2, 3j, 4]))
except Exception as e:
    print(e)
    print("caught exception as expected.")
#

try:
    print(tomographer.testgetmatrix_d(np.array([1.0,2.0,3,1e-5])))
except Exception as e:
    print(e)
    print("Caught exception as expected.")
