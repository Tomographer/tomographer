
rem We need to have a fresh clone of tomographer available, with a VERSION file ready.

set "PATH=C:\WINDOWS\system32;C:\WINDOWS;C:\WINDOWS\System32\Wbem;C:\Program Files\CMake 2.8\bin;Z:\MinGW\bin"

cd tomographer
mkdir build
cd build
cmake .. -G"MinGW Makefiles" ^
  -DCMAKE_MAKE_PROGRAM=Z:/MinGW/bin/mingw32-make.exe ^
  -DCMAKE_C_COMPILER=Z:/MinGW/bin/gcc.exe ^
  -DCMAKE_CXX_COMPILER=Z:/MinGW/bin/g++.exe ^
  -DCMAKE_BUILD_TYPE=Release ^
  -DCMAKE_INSTALL_PREFIX=Z:/tomographer-dist-admin/win-mingw32/tomographer/build/tomographer-INSTALL ^
  -DEIGEN3_INCLUDE_DIR=Z:/Eigen/eigen-eigen-07105f7124f9 ^
  -DMATIO_INCLUDE_DIR=Z:/MatIO/matio-1.5.6/src ^
  -DMATIO_LIBRARY=Z:/MatIO/matio-1.5.6/src/libmatio.a ^
  -DBOOST_INCLUDEDIR=Z:/Boost/boost_1_61_0 ^
  -DBoost_PROGRAM_OPTIONS_LIBRARY=Z:/Boost/boost_1_61_0/libs/program_options/src/libboost_program_options.a ^
  -DBoost_PROGRAM_OPTIONS_LIBRARY_RELEASE=Z:/Boost/boost_1_61_0/libs/program_options/src/libboost_program_options.a ^
  -DZLIB_INCLUDE_DIR=Z:/MinGW/include ^
  -DZLIB_LIBRARY=Z:/MinGW/lib/libz.a ^
  -DTARGET_ARCHITECTURE=none ^
  "-DCMAKE_CXX_FLAGS_RELEASE=-O2 -msse2 -Wall -Wextra"

mingw32-make VERBOSE=1

mingw32-make install
