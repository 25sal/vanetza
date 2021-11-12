# - Find libjsoncpp
# Find the native libjsoncpp includes and libraries
#
#  JSONCPP_INCLUDE_DIR - where to find json.h, etc.
#  JSONCPP_LIBRARIES   - List of libraries when using libjsoncpp.
#  JSONCPP_FOUND       - True if libjsoncpp found.

if (NOT JSONCPP_INCLUDE_DIR)
  find_path(JSONCPP_INCLUDE_DIR json/json.h)
endif()

if (NOT JSONCPP_LIBRARY)
  find_library(
    JSONCPP_LIBRARY
    NAMES jsoncpp)
endif()

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(
  JSONCPP DEFAULT_MSG
  JSONCPP_LIBRARY JSONCPP_INCLUDE_DIR)

message(STATUS "libjsoncpp include dir: ${JSONCPP_INCLUDE_DIR}")
message(STATUS "libjsoncpp: ${JSONCPP_LIBRARY}")
set(JSONCPP_LIBRARIES ${JSONCPP_LIBRARY})

mark_as_advanced(JSONCPP_INCLUDE_DIR JSONCPP_LIBRARY)
