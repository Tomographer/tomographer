#!/usr/bin/env python

import tomographer
import numpy as np

tomographer.test_eigen(np.array([1, 2, 3, 4]))
tomographer.test_eigen(np.array([[1.0,2.0],[3, 1e-5]]))

print tomographer.test_eigen2(np.array([[1.0,2.0],[3, 1e-5]]))
print tomographer.test_eigen2(np.array([[1.0j,2.0j],[3, 1e-5]]))

