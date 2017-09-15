
#
# First, we set up the environment variables we might need later
#


function nproc { sysctl -n hw.ncpu; }

#
# temp directory with short path name
#
TMPDIR=/tmp/zzzzxyyy-$$
mkdir -p $TMPDIR


#
# C/C++ compiler
#
if [[ "$TT_CC" == "clang-osx" ]]; then
    
    export CMAKE_C_COMPILER="/usr/bin/clang"
    export CMAKE_CXX_COMPILER="/usr/bin/clang++"
    
elif [[ "$TT_CC" == "gcc-conda" ]]; then
    
    # use gcc/g++ compiler provided by conda
    export CMAKE_C_COMPILER="$HOME/.miniconda/bin/gcc"
    export CMAKE_CXX_COMPILER="$HOME/.miniconda/bin/g++"
    export CMAKE_ADD_ARGS="$CMAKE_ADD_ARGS -DCMAKE_INSTALL_RPATH=$HOME/.miniconda/lib -DCMAKE_BUILD_WITH_INSTALL_RPATH=1"

elif [[ "$TT_CC" == "brew-gcc-7" ]]; then
    
    export CMAKE_C_COMPILER="/usr/local/opt/gcc/bin/gcc-7"
    export CMAKE_CXX_COMPILER="/usr/local/opt/gcc/bin/g++-7"
    export CUSTOM_BIN_INSTALL_MACOSX_BOOST_GCC7=1

else
    
    echo &>2 "TOMOGRAPHER TRAVIS TEST SETUP ERROR: Unknown TT_CC=$TT_CC"
    echo &>2 " --> Please edit top of test/travis/travis-install.sh"
    exit 127
    
fi

echo "[OSX] Using compilers -- C: $CMAKE_C_COMPILER,  C++: $CMAKE_CXX_COMPILER"

#
# Python version to use
#
if [[ "$TT_PYTHON" == "brew-python-3" ]]; then

    export PYTHON_EXECUTABLE=/usr/local/opt/python3/bin/python3
    export CMAKE_ADD_ARGS="$CMAKE_ADD_ARGS -DPYTHON_LIBRARY=/usr/local/opt/python3/Frameworks/Python.framework/Versions/3.6/lib/libpython3.6m.dylib"
    export PIP=/usr/local/bin/pip3
    export PIP_MAYBE_SUDO=""
    export PIP_DEPS_USE_USER="0"
    export INSTALL_PYTHON_DEPS_USING="pip"

elif [[ "$TT_PYTHON" == "conda-python-2.7" ]]; then

    export MINICONDA_INSTALLER=https://repo.continuum.io/miniconda/Miniconda2-latest-MacOSX-x86_64.sh
    export PYTHON_EXECUTABLE=$HOME/.miniconda/bin/python
    export CMAKE_ADD_ARGS="$CMAKE_ADD_ARGS -DCMAKE_PREFIX_PATH=$HOME/.miniconda -DPYTHON_LIBRARY=$HOME/.miniconda/lib/libpython2.7.so -DPYTHON_INCLUDE_DIR=$HOME/.miniconda/include/python2.7 "
    export PIP=$HOME/.miniconda/bin/pip
    export INSTALL_PYTHON_DEPS_USING="conda"

    # use boost from conda, hide this one
    brew unlink boost

elif [[ "$TT_PYTHON" == "osxsystem-python-2.7" ]]; then

    export PYTHON_EXECUTABLE=/usr/bin/python
    export CMAKE_ADD_ARGS="$CMAKE_ADD_ARGS -DPYTHON_LIBRARY=/System/Library/Frameworks/Python.framework/Versions/2.7/lib/libpython2.7.dylib"
    export PIP="/usr/bin/python -m pip"
    export PIP_MAYBE_SUDO="sudo -H"
    export PIP_DEPS_USE_USER="1"
    export INSTALL_PYTHON_DEPS_USING="pip"

    # install pip for system python
    sudo /usr/bin/easy_install pip

else

    echo &>2 "TOMOGRAPHER TRAVIS TEST SETUP ERROR: Unknown TT_PYTHON=$TT_PYTHON"
    echo &>2 " --> Please edit top of test/travis/travis-install.sh"
    exit 127

fi






#
# Install dependencies & set up the environment
#


pwd
export OUR_TRAVIS_PATH=`pwd`

# all (non-python) dependencies are normally installed via homebrew

export EIGEN_PATH=/usr/local/opt/eigen/include/eigen3
export MATIO_PREFIX_PATH=/usr/local/opt/libmatio
export MATIO_LIBRARY=$MATIO_PREFIX_PATH/lib/libmatio.dylib
export CMAKE_PATH=/usr/local/opt/cmake
export PYBIND11_PREFIX_PATH=/usr/local/opt/pybind11


# ... except this -- we might need custom version of boost compiled with g++-7
if [[ "$CUSTOM_BIN_INSTALL_MACOSX_BOOST_GCC7" == 1 ]]; then

    curl -L macosx-gcc7-boost-bin.tar.bz2 https://github.com/Tomographer/tomographer/raw/tomographer-ci-bin/macosx-gcc7-boost-bin.tar.bz2 | tar xj
    export CMAKE_ADD_ARGS="$CMAKE_ADD_ARGS -DBOOST_INCLUDEDIR=`pwd`/gcc7-env/include -DBOOST_ROOT=`pwd`/gcc7-env -DBOOST_LIBRARYDIR=`pwd`/gcc7-env/lib"

    # debug
    echo "Installed custom boost binaries for Mac OSX / gcc-7"
    ls `pwd`/gcc7-env/lib

fi




# Python dependencies
# hack for preventing travis to interrupt after 10min of no output -- see
# https://github.com/travis-ci/travis-ci/issues/6591
function write_visual_bells() { set +x; while true; do echo -en "\a"; sleep 10; done; }; write_visual_bells &


if [[ "$INSTALL_PYTHON_DEPS_USING" == "pip" ]]; then

    # system in any case
    $PIP_MAYBE_SUDO $PIP install --upgrade pip
    #$PIP_MAYBE_SUDO $PIP install --upgrade setuptools  # fails for system python ... :(
    $PIP_MAYBE_SUDO $PIP install --upgrade wheel

    $PIP_MAYBE_SUDO $PIP --version
    $PIP_MAYBE_SUDO $PIP show pip setuptools wheel

    $PIP_MAYBE_SUDO $PIP install numpy scipy >pip_output.txt 2>&1 || cat pip_output.txt

    # PIP_MAYBE_USER refers to these packages only ... especially pybind11
    if [[ "$PIP_DEPS_USE_USER" == "1" ]]; then
        $PIP install pybind11 --user
        $PIP install $PIP_EXTRAS cvxpy --user >pip_output.txt 2>&1 || cat pip_output.txt
    else
        $PIP_MAYBE_SUDO $PIP install pybind11
        $PIP_MAYBE_SUDO $PIP install $PIP_EXTRAS cvxpy >pip_output.txt 2>&1 || cat pip_output.txt
    fi

elif [[ "$INSTALL_PYTHON_DEPS_USING" == "conda" ]]; then

    wget "$MINICONDA_INSTALLER" -O miniconda.sh
    
    bash miniconda.sh -b -p "$HOME/.miniconda"
    export PATH="$HOME/.miniconda/bin:$PATH"

    hash -r

    conda config --set always_yes yes --set changeps1 no

    #conda config --add channels conda-forge
    #conda config --add channels cvxgrp

    conda update -q conda
    # Useful for debugging any issues with conda
    conda info -a

    # MKL gives us problems ... ARRRRRGHHHH!!!!
    #conda install nomkl

    conda install libgcc libgfortran

    conda install gcc

    # ### why bother with environments?
    # conda create -q -n test-environment python=3.6 pip ...

    conda install boost

    conda install numpy scipy matplotlib ecos
    conda install -f numpy
    conda install -c cvxgrp scs multiprocess cvxcanon cvxpy
    conda install -c conda-forge pybind11

    #source activate test-environment

    python -c 'import pybind11; print("pybind11 version: {}".format(pybind11.__version__))'
    python -c 'import numpy; print("NUMPY version: {}".format(numpy.__version__))'
    python -c 'import scs; print("SCS version: {}".format(scs.__version__))'
    python -c 'import cvxpy; print("CVXPY version: {}".format(cvxpy.__version__))'
    python -c 'import matplotlib; print("MATPLOTLIB version: {}".format(matplotlib.__version__))'

    ls -lh $HOME/.miniconda/lib/

    export PYBIND11_PREFIX_PATH=`python -c 'import pybind11; print(pybind11.get_include())'`

else

    echo >&2 "Error: INSTALL_PYTHON_DEPS_USING=$INSTALL_PYTHON_DEPS_USING  unknown !!!"
    exit 123

fi



