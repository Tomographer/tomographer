
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

dim = 2**6

# Emn formatted for MATLAB file, column-major
matEmn = np.zeros(shape=(dim,dim,2))
matEmn[:,:,0] = proj_W
matEmn[:,:,1] = np.eye(64) - proj_W

# Emn formatted for Python NumPy, Emn[k,:,:] is POVM effect
Emn = [ proj_W,
        np.eye(64) - proj_W ]

# let's plug in some simulated data
Nm = [ 9991, 9 ]
matNm = np.array([Nm]).T

if __name__ == '__main__':
    sio.savemat('data_file.mat', {'dim': dim, 'Emn': matEmn, 'Nm': matNm, 'proj_W': proj_W })
