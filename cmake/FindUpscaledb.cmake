#
#  UPSCALEDB_FOUND - system has upscaledb
#  UPSCALEDB_INCLUDE_DIR - the upscaledb include directory
#  UPSCALEDB_LIBRARIES - upscaledb library

set(UPSCALEDB_LIBRARY_DIRS "")

find_library(UPSCALEDB_LIBRARIES NAMES upscaledb)
find_path(UPSCALEDB_INCLUDE_DIRS NAMES ups/upscaledb.h)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(UPSCALEDB DEFAULT_MSG UPSCALEDB_LIBRARIES UPSCALEDB_INCLUDE_DIRS)
