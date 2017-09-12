
set -x

mkdir build
cd build

#
# Run CMAKE
#

$CMAKE_PATH/bin/cmake .. -DCMAKE_C_COMPILER=$CMAKE_C_COMPILER -DCMAKE_CXX_COMPILER=$CMAKE_CXX_COMPILER $CMAKE_ADD_ARGS -DBUILD_TOMOPY=on -DTOMOGRAPHER_ENABLE_TESTS=on -DTOMOGRAPHER_TEST_SETUPPY_BUILD=on -DEIGEN3_INCLUDE_DIR="$EIGEN_PATH" -DMATIO_INCLUDE_DIR="$MATIO_PREFIX_PATH/include" -DMATIO_LIBRARY="$MATIO_LIBRARY" -DCMAKE_FIND_ROOT_PATH="$PYBIND11_PREFIX_PATH" -DPYTHON_EXECUTABLE=$PYTHON_EXECUTABLE  || cat build/CMakeFiles/CMakeError.log

#
# Run MAKE
#
# make tomorun separately, one process only, because it's VERY heavy on RAM
make tomorun VERBOSE=1 || exit 1
# then make the rest using all available cores
make -j`nproc` VERBOSE=1 || exit 1

#
# use python/setup.py to build a source package and to compile the extension a
# first time -- just to make sure it works
#
(cd ../py; CC=$CMAKE_C_COMPILER CXX=$CMAKE_CXX_COMPILER CMAKE_CACHE_FILE=../build/CMakeCache.txt $PYTHON_EXECUTABLE setup.py sdist build bdist_egg bdist_wheel || exit 1 ) || exit 1

#
# make sure that pip can compile & install from the sdist package.  This is the package
# we want to test in our CTest suite as "setuppy-built".
#
CC=$CMAKE_C_COMPILER CXX=$CMAKE_CXX_COMPILER $PIP install --user ../py/dist/tomographer-*.tar.gz  || exit 1

CTEST_OUTPUT_ON_FAILURE=1 BOOST_TEST_LOG_LEVEL=all $CMAKE_PATH/bin/ctest --timeout 480  || exit 1


#
# Make sure that other custom python modules compile against our tomographer python package
#
(cd ../doc/py/my_custom_module/; CC=$CMAKE_C_COMPILER CXX=$CMAKE_CXX_COMPILER $PYTHON_EXECUTABLE setup.py sdist bdist_wheel || exit 1) || exit 1
# and make sure we can compile it from pip, and run it
CC=$CMAKE_C_COMPILER CXX=$CMAKE_CXX_COMPILER $PIP install --user ../doc/py/my_custom_module/dist/my_custom_package*.tar.gz || exit 1
$PYTHON_EXECUTABLE -c 'import my_custom_module; print(my_custom_module.__version__)' || exit 1

set +x
