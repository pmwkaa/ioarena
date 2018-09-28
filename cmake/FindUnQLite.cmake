#
#  UNQLITE_FOUND - system has UnQLite
#  UNQLITE_INCLUDE_DIR - the UnQLite include directory
#  UNQLITE_LIBRARIES - UnQLite library

set(UNQLITE_LIBRARY_DIRS "")

find_library(UNQLITE_LIBRARIES NAMES unqlite)
find_path(UNQLITE_INCLUDE_DIRS NAMES unqlite.h)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(UNQLITE DEFAULT_MSG UNQLITE_LIBRARIES UNQLITE_INCLUDE_DIRS)
