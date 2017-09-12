
#
# First, we set up the environment variables we might need later
#



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
    export INSTALL_PYTHON_DEPS_USING="brew-pip"

elif [[ "$TT_PYTHON" == "conda-python-2.7" ]]; then

    export MINICONDA_INSTALLER=https://repo.continuum.io/miniconda/Miniconda2-latest-MacOSX-x86_64.sh
    export PYTHON_EXECUTABLE=$HOME/.miniconda/bin/python
    export CMAKE_ADD_ARGS="$CMAKE_ADD_ARGS -DCMAKE_PREFIX_PATH=$HOME/.miniconda/envs/test-environment -DPYTHON_LIBRARY=$HOME/.miniconda/lib/libpython2.7.so -DPYTHON_INCLUDE_DIR=$HOME/.miniconda/include/python2.7 "
    export PIP=$HOME/.miniconda/bin/pip
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

# all dependencies via homebrew for now -- no need to download & install anything manually

export EIGEN_PATH=/usr/local/opt/eigen/include/eigen3
export MATIO_PREFIX_PATH=/usr/local/opt/libmatio
export MATIO_LIBRARY=$MATIO_PREFIX_PATH/lib/libmatio.dylib
export CMAKE_PATH=/usr/local/opt/cmake
export PYBIND11_PREFIX_PATH=/usr/local/opt/pybind11

# # Eigen3: need >= 3.3
# export EIGEN_PATH=`pwd`/eigen-3.3.1
# if [[ ! -f "$EIGEN_PATH/Eigen/src/Core/util/Macros.h" ]]; then curl -L https://bitbucket.org/eigen/eigen/get/3.3.1.tar.gz -o eigen-3.3.1.tar.gz && tar xfz eigen-3.3.1.tar.gz && mkdir -p eigen-3.3.1 && mv eigen-eigen-f562a193118d/* eigen-3.3.1 ; fi

# # MatIO:
# export MATIO_PATH=`pwd`/matio-1.5.9
# if [[ ! -f "$MATIO_PATH/_install/include/matio.h" ]]; then curl -L -o matio-1.5.9.tar.gz  https://sourceforge.net/projects/matio/files/matio/1.5.9/matio-1.5.9.tar.gz/download  && tar xfz matio-1.5.9.tar.gz && ( cd matio-1.5.9 && ./configure --prefix="$MATIO_PATH/_install" && make && make install ) ; fi

# # CMake:
# export CMAKE_PATH=`pwd`/cmake-3.1.3-Linux-x86_64
# if [[ ! -f "$CMAKE_PATH/bin/cmake" ]]; then curl -L -O https://cmake.org/files/v3.1/cmake-3.1.3-Linux-x86_64.tar.gz && tar xfz cmake-3.1.3-Linux-x86_64.tar.gz ; fi

# # PyBind11:
# # patch -- incorporate change https://github.com/pybind/pybind11/pull/1062 which
# #          fixes compilation using clang & libstdc++ on gcc==4
# export PYBIND11_PATH=`pwd`/pybind11-2.2.0
# if [[ ! -f "$PYBIND11_PATH/_install/include/pybind11/pybind11.h" ]]; then curl -L https://github.com/pybind/pybind11/archive/v2.2.0.tar.gz | tar xz && patch -p0 <$OUR_TRAVIS_PATH/test/travis/fix_clang-libstdcxx-gcc4_for_pybind-2-2-0.patch && (mkdir -p "$PYBIND11_PATH/build" && cd "$PYBIND11_PATH/build" && cmake .. -DPYBIND11_TEST=0 -DPYTHON_EXECUTABLE=$PYTHON_EXECUTABLE -DCMAKE_INSTALL_PREFIX="$PYBIND11_PATH/_install" && make install) ; fi




# Python dependencies
# hack for preventing travis to interrupt after 10min of no output -- see
# https://github.com/travis-ci/travis-ci/issues/6591
function write_visual_bells() { set +x; while true; do echo -en "\a"; sleep 10; done; }; write_visual_bells &


if [[ "$INSTALL_PYTHON_DEPS_USING" == "brew-pip" ]]; then

    $PIP install --upgrade pip
    $PIP install wheel

    # for some reason we need sudo to install pybind11 (??)
    $PIP install pybind11

    if [[ "$TT_CC" =~ ^clang.*$ ]]; then
        #
        # PATCH pybind11 for compilation with clang using libstdc++ on gcc-4.8
        #
        pybind11include=`$PYTHON_EXECUTABLE -c 'import pybind11; assert(pybind11.__version__ == "2.2.0"); print(pybind11.get_include());'`
        (cd "$pybind11include" && sudo patch -p2 <$OUR_TRAVIS_PATH/test/travis/fix_clang-libstdcxx-gcc4_for_pybind-2-2-0.patch)
    fi

    $PIP install $PIP_EXTRAS numpy scipy cvxpy >pip_output.txt 2>&1 || cat pip_output.txt

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

    if [[ "$TT_CC" =~ ^clang.*$ ]]; then
        #
        # PATCH pybind11 for compilation with clang using libstdc++ on gcc-4.8
        #
        pybind11include=`$PYTHON_EXECUTABLE -c 'import pybind11; assert(pybind11.__version__ == "2.2.0"); print(pybind11.get_include());'`
        (cd "$pybind11include" && patch -p2 <test/travis/fix_clang-libstdcxx-gcc4_for_pybind-2-2-0.patch)
    fi

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



