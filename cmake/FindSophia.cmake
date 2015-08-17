#
#  SOPHIA_FOUND - system has libsophia
#  SOPHIA_INCLUDE_DIR - the libsophia include directory
#  SOPHIA_LIBRARIES - libsophia library

set (SOPHIA_LIBRARY_DIRS "")

find_library(SOPHIA_LIBRARIES NAMES sophia)
find_path(SOPHIA_INCLUDE_DIRS NAMES sophia.h)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(SOPHIA DEFAULT_MSG SOPHIA_LIBRARIES SOPHIA_INCLUDE_DIRS)

#mark_as_advanced(SOPHIA_INCLUDE_DIRS SOPHIA_LIBRARIES)
