#
#  LMDB_FOUND - system has liblmdb
#  LMDB_INCLUDE_DIR - the liblmdb include directory
#  LMDB_LIBRARIES - liblmdb library

set (LMDB_LIBRARY_DIRS "")

find_library(LMDB_LIBRARIES NAMES lmdb)
find_path(LMDB_INCLUDE_DIRS NAMES lmdb.h)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(LMDB DEFAULT_MSG LMDB_LIBRARIES LMDB_INCLUDE_DIRS)

#mark_as_advanced(LMDB_INCLUDE_DIRS LMDB_LIBRARIES)
