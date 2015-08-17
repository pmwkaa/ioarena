#
#  ROCKSDB_FOUND - system has librocksdb
#  ROCKSDB_INCLUDE_DIR - the librocksdb include directory
#  ROCKSDB_LIBRARIES - librocksdb library

set (ROCKSDB_LIBRARY_DIRS "")

find_library(ROCKSDB_LIBRARIES NAMES rocksdb)
find_path(ROCKSDB_INCLUDE_DIRS NAMES rocksdb/c.h)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(ROCKSDB DEFAULT_MSG ROCKSDB_LIBRARIES ROCKSDB_INCLUDE_DIRS)

#mark_as_advanced(ROCKSDB_INCLUDE_DIRS ROCKSDB_LIBRARIES)
