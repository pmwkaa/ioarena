#
#  SQLITE3_FOUND - system has libsqlite3
#  SQLITE3_INCLUDE_DIR - the libsqlite3 include directory
#  SQLITE3_LIBRARIES - libsqlite3 library

set (SQLITE3_LIBRARY_DIRS "")

find_library(SQLITE3_LIBRARIES NAMES sqlite3)
find_path(SQLITE3_INCLUDE_DIRS NAMES sqlite3.h)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(SQLITE3 DEFAULT_MSG SQLITE3_LIBRARIES SQLITE3_INCLUDE_DIRS)

#mark_as_advanced(SQLITE3_INCLUDE_DIRS SQLITE3_LIBRARIES)
