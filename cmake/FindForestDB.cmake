#
#  FORESTDB_FOUND - system has libforestdb
#  FORESTDB_INCLUDE_DIR - the libforestdb include directory
#  FORESTDB_LIBRARIES - libforestdb library

set (FORESTDB_LIBRARY_DIRS "")

find_library(FORESTDB_LIBRARIES NAMES forestdb)
find_path(FORESTDB_INCLUDE_DIRS NAMES forestdb.h)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(FORESTDB DEFAULT_MSG FORESTDB_LIBRARIES FORESTDB_INCLUDE_DIRS)

#mark_as_advanced(FORESTDB_INCLUDE_DIRS FORESTDB_LIBRARIES)
