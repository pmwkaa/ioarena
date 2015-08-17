#
#  WIREDTIGER_FOUND - system has libwiredtiger
#  WIREDTIGER_INCLUDE_DIR - the libwiredtiger include directory
#  WIREDTIGER_LIBRARIES - libwiredtiger library

set (WIREDTIGER_LIBRARY_DIRS "")

find_library(WIREDTIGER_LIBRARIES NAMES wiredtiger)
find_path(WIREDTIGER_INCLUDE_DIRS NAMES wiredtiger.h)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(WIREDTIGER DEFAULT_MSG WIREDTIGER_LIBRARIES WIREDTIGER_INCLUDE_DIRS)

#mark_as_advanced(WIREDTIGER_INCLUDE_DIRS WIREDTIGER_LIBRARIES)
