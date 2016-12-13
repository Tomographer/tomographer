
#from . import jpyutil as _jpyutil
#from . import querrorbars as _querrorbars
#def __bootstrap__():
#   global __bootstrap__, __loader__, __file__
#   import sys, pkg_resources, imp
#   __file__ = imp.find_module('_tomographer_cxx')[1]
#   __loader__ = None;
#   del __bootstrap__, __loader__
#   imp.load_dynamic(__name__, __file__)
#__bootstrap__()
#jpyutil = _jpyutil
#querrorbars = _querrorbars

# load the C++ module and import the relevant symbols here
from _tomographer_cxx import *

import _tomographer_cxx
__version__ = _tomographer_cxx.__version__

# set up docstrings for our classes
#from . import _docstrings
