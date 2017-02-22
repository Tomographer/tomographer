# the name of the target operating system
SET(CMAKE_SYSTEM_NAME Windows)

SET(CMAKE_SYSTEM_PROCESSOR i686)

# which compilers to use for C and C++
SET(CMAKE_C_COMPILER /usr/bin/i686-w64-mingw32-gcc-win32)
SET(CMAKE_CXX_COMPILER /usr/bin/i686-w64-mingw32-g++-win32)
SET(CMAKE_RC_COMPILER /usr/bin/i686-w64-mingw32-windres)

# here is the target environment located
SET(CMAKE_FIND_ROOT_PATH /usr/i686-w64-mingw32 /media/sf_vmshare/mingw32-w64-sandbox/local )
SET(CMAKE_SYSROOT )

# adjust the default behaviour of the FIND_XXX() commands:
# search headers and libraries in the target environment, search 
# programs in the host environment
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
