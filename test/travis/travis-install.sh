
#
# First, we set up the environment variables we might need later
#

set -x

#
# C/C++ compiler
#
if [ "$TT_CC" == "gcc-4.8" ]; then
    
    export CMAKE_C_COMPILER="gcc-4.8"
    export CMAKE_CXX_COMPILER="g++-4.8"
    
elif [ "$TT_CC" == "gcc-5" ]; then
    
    export CMAKE_C_COMPILER="gcc-5"
    export CMAKE_CXX_COMPILER="g++-5"
    
elif [ "$TT_CC" == "gcc-6" ]; then
    
    export CMAKE_C_COMPILER="gcc-6"
    export CMAKE_CXX_COMPILER="g++-6"
    
elif [ "$TT_CC" == "clang-3.4" ]; then
    
    export CMAKE_C_COMPILER="clang-3.4"
    export CMAKE_CXX_COMPILER="clang++-3.4"
    
elif [ "$TT_CC" == "clang-3.8" ]; then
    
    export CMAKE_C_COMPILER="clang-3.8"
    export CMAKE_CXX_COMPILER="clang++-3.8"
    
elif [ "$TT_CC" == "conda-3" ]; then # not used in the end
    
    # use gcc/g++ compiler provided by conda
    export CMAKE_C_COMPILER="$HOME/.miniconda/envs/test-environment/bin/gcc"
    export CMAKE_CXX_COMPILER="$HOME/.miniconda/envs/test-environment/bin/g++"
    export CMAKE_ADD_ARGS="$CMAKE_ADD_ARGS  -DZLIB_LIBRARY=/usr/lib/x86_64-linux-gnu/libz.so -DBoost_PROGRAM_OPTIONS_LIBRARY_RELEASE=/usr/lib/x86_64-linux-gnu/libboost_program_options.a -DBoost_UNIT_TEST_FRAMEWORK_LIBRARY_RELEASE=/usr/lib/x86_64-linux-gnu/libboost_unit_test_framework.so -DBoost_SERIALIZATION_LIBRARY_RELEASE=/usr/lib/x86_64-linux-gnu/libboost_serialization.so "
    
else
    
    echo &>2 "TOMOGRAPHER TRAVIS TEST SETUP ERROR: Unknown TT_CC=$TT_CC"
    echo &>2 " --> Please edit top of test/travis/travis-install.sh"
    exit 127
    
fi

echo "Using compilers -- C: $CMAKE_C_COMPILER,  C++: $CMAKE_CXX_COMPILER"

#
# Python version to use
#
if [ "$TT_PYTHON" == "system-python-2.7" ]; then

    export PYTHON_EXECUTABLE=/usr/bin/python
    export CMAKE_ADD_ARGS="$CMAKE_ADD_ARGS -DPYTHON_LIBRARY=/usr/lib/x86_64-linux-gnu/libpython2.7.so -DPYTHON_INCLUDE_DIR=/usr/include/python2.7"
    export PIP=/usr/bin/pip
    export INSTALL_PYTHON_DEPS_USING="pip"

elif [ "$TT_PYTHON" == "system-python-3" ]; then

    export PYTHON_EXECUTABLE=/usr/bin/python3
    #export CMAKE_ADD_ARGS="$CMAKE_ADD_ARGS "  # no special CMake args required
    export PIP=/usr/bin/pip3
    export INSTALL_PYTHON_DEPS_USING="pip"

elif [ "$TT_PYTHON" == "conda-3" ]; then

    export MINICONDA_INSTALLER=https://repo.continuum.io/miniconda/Miniconda3-latest-Linux-x86_64.sh
    export PYTHON_EXECUTABLE=$HOME/.miniconda/envs/test-environment/bin/python
    export CMAKE_ADD_ARGS="$CMAKE_ADD_ARGS -DCMAKE_PREFIX_PATH=$HOME/.miniconda/envs/test-environment -DPYTHON_LIBRARY=$HOME/.miniconda/envs/test-environment/lib/libpython3.6m.so -DPYTHON_INCLUDE_DIR=$HOME/.miniconda/envs/test-environment/include/python3.6m "
    export PIP=$HOME/.miniconda/envs/test-environment/bin/pip
    export INSTALL_PYTHON_DEPS_USING="conda"

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

# Eigen3: need >= 3.3
export EIGEN_PATH=`pwd`/eigen-3.3.1
if [ ! -f "$EIGEN_PATH/Eigen/src/Core/util/Macros.h" ]; then curl -L https://bitbucket.org/eigen/eigen/get/3.3.1.tar.gz -o eigen-3.3.1.tar.gz && tar xfz eigen-3.3.1.tar.gz && mkdir -p eigen-3.3.1 && mv eigen-eigen-f562a193118d/* eigen-3.3.1 ; fi

# MatIO:
export MATIO_PATH=`pwd`/matio-1.5.9
if [ ! -f "$MATIO_PATH/_install/include/matio.h" ]; then curl -L -o matio-1.5.9.tar.gz  https://sourceforge.net/projects/matio/files/matio/1.5.9/matio-1.5.9.tar.gz/download  && tar xfz matio-1.5.9.tar.gz && ( cd matio-1.5.9 && ./configure --prefix="$MATIO_PATH/_install" && make && make install ) ; fi

# CMake:
export CMAKE_PATH=`pwd`/cmake-3.1.3-Linux-x86_64
if [ ! -f "$CMAKE_PATH/bin/cmake" ]; then curl -L -O https://cmake.org/files/v3.1/cmake-3.1.3-Linux-x86_64.tar.gz && tar xfz cmake-3.1.3-Linux-x86_64.tar.gz ; fi

# PyBind11:
export PYBIND11_PATH=`pwd`/pybind11-2.1.0
if [ ! -f "$PYBIND11_PATH/_install/include/pybind11/pybind11.h" ]; then curl -L https://github.com/pybind/pybind11/archive/v2.1.0.tar.gz | tar xz && (mkdir -p "$PYBIND11_PATH/build" && cd "$PYBIND11_PATH/build" && cmake .. -DPYBIND11_TEST=0 -DCMAKE_INSTALL_PREFIX="$PYBIND11_PATH/_install" && make install) ; fi


# Python dependencies
# hack for preventing travis to interrupt after 10min of no output -- see
# https://github.com/travis-ci/travis-ci/issues/6591
function write_visual_bells() { set +x; while true; do echo -en "\a"; sleep 10; done; }; write_visual_bells &


if [ "$INSTALL_PYTHON_DEPS_USING" == "pip" ]; then

    # needed for pip upgrade, ...
    sudo -H $PIP install urllib3[secure]
    # ... upgrade which is needed for --cache-dir :( ...
    $PIP install --user --upgrade pip
    # ... so that we can finally run pip


    (mkdir -p pip_sandbox && cd pip_sandbox && CC=${PIP_CC=gcc} CXX=${PIP_CXX=g++} ~/.local/bin/pip install --cache-dir=$OUR_TRAVIS_PATH/pip_cache/$PYTHON_EXECUTABLE --user $PIP_EXTRAS wheel cvxpy pybind11 >pip_output.txt 2>&1 || cat pip_output.txt )

elif [ "$INSTALL_PYTHON_DEPS_USING" == "conda" ]; then

    # see https://conda.io/docs/user-guide/tasks/use-conda-with-travis-ci.html

    sudo apt-get update

    wget "$MINICONDA_INSTALLER" -O miniconda.sh
    
    bash miniconda.sh -b -p "$HOME/.miniconda"
    export PATH="$HOME/.miniconda/bin:$PATH"

    hash -r

    conda config --set always_yes yes --set changeps1 no

    conda config --add channels conda-forge
    conda config --add channels cvxgrp
    #conda config --add channels salford_systems # gcc-5

    conda update -q conda
    # Useful for debugging any issues with conda
    conda info -a

    conda create -q -n test-environment python=3.6 libgcc libgfortran lapack openblas pybind11 wheel numpy scipy matplotlib tk scs cvxpy
    source activate test-environment

    python -c 'import scs; print("SCS version: {}".format(scs.__version__))'
    python -c 'import cvxpy; print("CVXPY version: {}".format(cvxpy.__version__))'

    ls -lh $HOME/.miniconda/envs/test-environment/lib/

else

    echo >&2 "Error: INSTALL_PYTHON_DEPS_USING=$INSTALL_PYTHON_DEPS_USING  unknown !!!"
    exit 123

fi

set +x
