
#
# Create a MATLAB data file for tomorun, for a simulated example of six qubits in a W
# state measured with the two-outcome POVM { |W><W|, 1-|W><W| } .
#

import numpy as np
import scipy.io as sio


psi_W = np.zeros(shape=(2**6,1))
psi_W[1] = 1.0/np.sqrt(6.0)
psi_W[2] = 1.0/np.sqrt(6.0)
psi_W[4] = 1.0/np.sqrt(6.0)
psi_W[8] = 1.0/np.sqrt(6.0)
psi_W[16] = 1.0/np.sqrt(6.0)
psi_W[32] = 1.0/np.sqrt(6.0)


proj_W = np.outer(psi_W, psi_W.T.conj())


Emn = np.zeros(shape=(2**6,2**6,2))
Emn[:,:,0] = proj_W
Emn[:,:,1] = np.eye(64) - proj_W

# let's plug in some simulated data
Nm = np.array([[ 9991, 9 ]]).T

sio.savemat('data_file.mat', {'dim': 64, 'Emn': Emn, 'Nm': Nm, 'proj_W': proj_W })
