#
#  EJDB_FOUND - system has libejdb
#  EJDB_INCLUDE_DIR - the libejdb include directory
#  EJDB_LIBRARIES - libejdb library

set (EJDB_LIBRARY_DIRS "")

find_library(EJDB_LIBRARIES NAMES ejdb)
find_path(EJDB_INCLUDE_DIRS NAMES ejdb/ejdb.h)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(EJDB DEFAULT_MSG EJDB_LIBRARIES EJDB_INCLUDE_DIRS)

#mark_as_advanced(EJDB_INCLUDE_DIRS EJDB_LIBRARIES)
