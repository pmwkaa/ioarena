
macro(build_upscaledb)
  set(UPSCALEDB_INCLUDE_DIRS ${PROJECT_BINARY_DIR}/db/upscaledb/include)
  set(UPSCALEDB_OPTS CFLAGS="${CMAKE_C_FLAGS}" LDFLAGS="${CMAKE_SHARED_LINKER_FLAGS}")
  separate_arguments(UPSCALEDB_OPTS)
  if (${PROJECT_BINARY_DIR} STREQUAL ${PROJECT_SOURCE_DIR})
    add_custom_command(
      OUTPUT ${PROJECT_BINARY_DIR}/db/upscaledb/src/.libs/libupscaledb${CMAKE_SHARED_LIBRARY_SUFFIX}
      WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/db/upscaledb
      COMMAND autoreconf -if
      COMMAND ./configure --disable-java --disable-remote --without-tcmalloc --without-berkeleydb
      COMMAND ${CMAKE_MAKE_PROGRAM} -j ${MAKE_NJOB} ${UPSCALEDB_OPTS} -C ${PROJECT_BINARY_DIR}/db/upscaledb
      DEPENDS ${CMAKE_BINARY_DIR}/CMakeCache.txt
      )
  else()
    add_custom_command(
      OUTPUT ${PROJECT_BINARY_DIR}/db/upscaledb/src/.libs/libupscaledb${CMAKE_SHARED_LIBRARY_SUFFIX}
      WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/db/upscaledb
      COMMAND autoreconf -if
      COMMAND ./configure --disable-java --disable-remote --without-tcmalloc --without-berkeleydb
      COMMAND ${CMAKE_COMMAND} -E make_directory ${PROJECT_BINARY_DIR}/db/upscaledb
      COMMAND ${CMAKE_COMMAND} -E copy_directory ${PROJECT_SOURCE_DIR}/db/upscaledb ${PROJECT_BINARY_DIR}/db/upscaledb
      COMMAND ${CMAKE_MAKE_PROGRAM} -j ${MAKE_NJOB} ${UPSCALEDB_OPTS} -C ${PROJECT_BINARY_DIR}/db/upscaledb
      DEPENDS ${PROJECT_BINARY_DIR}/CMakeCache.txt
      )
  endif()
  add_custom_target(libupscaledb ALL
    DEPENDS ${PROJECT_BINARY_DIR}/db/upscaledb/src/.libs/libupscaledb${CMAKE_SHARED_LIBRARY_SUFFIX}
    )
  message(STATUS "Use shipped upscaledb: ${PROJECT_SOURCE_DIR}/db/upscaledb")
  set (UPSCALEDB_LIBRARIES "${PROJECT_BINARY_DIR}/db/upscaledb/src/.libs/libupscaledb${CMAKE_SHARED_LIBRARY_SUFFIX}")
  set (UPSCALEDB_FOUND 1)
  add_dependencies(build_libs libupscaledb)
endmacro(build_upscaledb)
