#
#  NessDB_FOUND - system has libnessdb
#  NessDB_INCLUDE_DIR - the libnessdb include directory
#  NessDB_LIBRARIES - libnessdb library

set (NESSDB_LIBRARY_DIRS "")

find_library(NESSDB_LIBRARIES NAMES nessdb)
find_path(NESSDB_INCLUDE_DIRS NAMES ness.h)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(NESSDB DEFAULT_MSG NESSDB_LIBRARIES NESSDB_INCLUDE_DIRS)

