
# set TOMOGRAPHER_VERSION
file(READ "${CMAKE_SOURCE_DIR}/VERSION" _tomover)
string(STRIP "${_tomover}" TOMOGRAPHER_VERSION)
