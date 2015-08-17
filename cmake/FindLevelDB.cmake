#
#  LEVELDB_FOUND - system has libleveldb
#  LEVELDB_INCLUDE_DIR - the libleveldb include directory
#  LEVELDB_LIBRARIES - libleveldb library

set (LEVELDB_LIBRARY_DIRS "")

find_library(LEVELDB_LIBRARIES NAMES leveldb)
find_path(LEVELDB_INCLUDE_DIRS NAMES leveldb/c.h)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(LEVELDB DEFAULT_MSG LEVELDB_LIBRARIES LEVELDB_INCLUDE_DIRS)

#mark_as_advanced(LEVELDB_INCLUDE_DIRS LEVELDB_LIBRARIES)
