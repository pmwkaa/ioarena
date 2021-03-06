
macro(build_mdbx)
  set(MDBX_INCLUDE_DIRS ${PROJECT_SOURCE_DIR}/db/mdbx)
  message(STATUS "Use shipped MDBX: ${PROJECT_SOURCE_DIR}/db/mdbx")
  set (MDBX_BUILD_SHARED_LIBRARY ON)
  set (MDBX_DISABLE_PAGECHECKS ON)
  set (MDBX_TXN_CHECKOWNER OFF)
  set (MDBX_ENV_CHECKPID OFF)
  add_subdirectory(db/mdbx ${PROJECT_BINARY_DIR}/db/mdbx)
  set (MDBX_LIBRARIES mdbx)
  set (MDBX_FOUND 1)
  add_dependencies(build_libs mdbx)
endmacro(build_mdbx)
