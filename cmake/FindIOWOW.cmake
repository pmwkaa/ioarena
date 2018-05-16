# Find the IOWOW headers and libraries, attemting to import IOWOW exported targets,
#
#  IOWOW_INCLUDE_DIRS - The IOWOW include directory
#  IOWOW_LIBRARIES    - The libraries needed to use IOWOW
#  IOWOW_FOUND        - True if IOWOW found in system

set (IOWOW_LIBRARY_DIRS "")

find_library(IOWOW_LIBRARIES NAMES iowow)
find_path(IOWOW_INCLUDE_DIRS NAMES iowow/iwkv.h)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(IOWOW DEFAULT_MSG IOWOW_LIBRARIES IOWOW_INCLUDE_DIRS)

