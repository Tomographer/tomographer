
# ==============================================================================
# Set up the build environment
# ==============================================================================

# Activate C++0x/C++11 support
if(COMPILER_SUPPORTS_CXX11)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
elseif(COMPILER_SUPPORTS_CXX0X)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x")
else()
  message(FATAL_ERROR
    "The compiler ${CMAKE_CXX_COMPILER} has no C++0x/C++11 support. Please use a different C++ compiler."
    )
endif()

# Access <Eigen/...> include path
include_directories(${EIGEN3_INCLUDE_DIR})

# Include <tomographer/...> include path. Note that the path needs to be relative to get
# the good automatic header dependencies
include_directories(.)



# ==============================================================================
# Include Subdirs
# ==============================================================================



# Tomographer library
add_subdirectory(tomographer)

# TomoRun executable
add_subdirectory(tomorun)

# Tests
add_subdirectory(test)
