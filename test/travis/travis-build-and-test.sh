
set -x

mkdir build
cd build

#
# Run CMAKE
#

$CMAKE_PATH/bin/cmake .. -DCMAKE_C_COMPILER=$CMAKE_C_COMPILER -DCMAKE_CXX_COMPILER=$CMAKE_CXX_COMPILER $CMAKE_ADD_ARGS -DBUILD_TOMOPY=on -DTOMOGRAPHER_ENABLE_TESTS=on -DTOMOGRAPHER_TEST_SETUPPY_BUILD=on -DEIGEN3_INCLUDE_DIR="$EIGEN_PATH" -DMATIO_INCLUDE_DIR="$MATIO_PATH/_install/include" -DMATIO_LIBRARY="$MATIO_PATH/_install/lib/libmatio.so" -DCMAKE_FIND_ROOT_PATH="$PYBIND11_PATH/_install" -DPYTHON_EXECUTABLE=$PYTHON_EXECUTABLE  || cat build/CMakeFiles/CMakeError.log

#
# Run MAKE
#
make VERBOSE=1 || exit 1

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


set +x
