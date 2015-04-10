cmake_minimum_required(VERSION 2.8.5)

project(tomographer)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

include(Util)

# default to release build with debugging info
if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "RelWithDebInfo")
endif()
message(STATUS "CMake Build Type: ${CMAKE_BUILD_TYPE}")


# ==============================================================================
# Find specific features and packages
# ==============================================================================

# Check for C++0x/C++11 features
include(CheckCXXCompilerFlag)
CHECK_CXX_COMPILER_FLAG("-std=c++11" COMPILER_SUPPORTS_CXX11)
CHECK_CXX_COMPILER_FLAG("-std=c++0x" COMPILER_SUPPORTS_CXX0X)

# Find Eigen3 package
find_package(Eigen3 REQUIRED)

# Find OpenMP
find_package(OpenMP)

# Find MatIO library (https://sourceforge.net/projects/matio/)
find_package(MatIO REQUIRED)

# ... which requires the libz library
find_package(ZLIB REQUIRED)

# Find boost::program_options
find_package(Boost 1.40 COMPONENTS program_options REQUIRED)


# activate warning flags on debug builds
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wall -Wextra")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} -Wall -Wextra")


# optimization flags for computer architecture.
# -> set TARGET_ARCHITECTURE=generic to disable optimizations
include(OptimizeForArchitecture)
OptimizeForArchitecture()
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${Vc_ARCHITECTURE_FLAGS}")


# ==============================================================================
# Set up Install Targets
# ==============================================================================

include(GNUInstallDirs)


# Set up uninstall target
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake_uninstall.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
    IMMEDIATE @ONLY)
add_custom_target(uninstall
    COMMAND ${CMAKE_COMMAND} -P ${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake
    )



# ==============================================================================
# Include our subdirectories
# ==============================================================================

add_subdirectory(cxx)



