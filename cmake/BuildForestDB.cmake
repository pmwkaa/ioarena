
macro(build_forestdb)
	set(FORESTDB_INCLUDE_DIRS ${PROJECT_SOURCE_DIR}/db/forestdb/include/libforestdb)
	set(FORESTDB_OPTS CFLAGS="${CMAKE_C_FLAGS}" LDFLAGS="${CMAKE_SHARED_LINKER_FLAGS}")
	separate_arguments(FORESTDB_OPTS)
	add_custom_command(
	    OUTPUT ${PROJECT_BINARY_DIR}/db/forestdb/mkdir
	    COMMAND ${CMAKE_COMMAND} -E make_directory ${PROJECT_BINARY_DIR}/db/forestdb
	    COMMAND ${CMAKE_COMMAND} -E touch ${PROJECT_BINARY_DIR}/db/forestdb/mkdir
	)
	add_custom_command(
		OUTPUT ${PROJECT_BINARY_DIR}/db/forestdb/libforestdb${CMAKE_SHARED_LIBRARY_SUFFIX}
		WORKING_DIRECTORY ${PROJECT_BINARY_DIR}/db/forestdb
		COMMAND ${CMAKE_COMMAND} ${PROJECT_SOURCE_DIR}/db/forestdb
		COMMAND ${CMAKE_MAKE_PROGRAM} -j ${MAKE_NJOB} ${FORESTDB_OPTS} forestdb
		DEPENDS ${PROJECT_BINARY_DIR}/CMakeCache.txt ${PROJECT_BINARY_DIR}/db/forestdb/mkdir
	)
	add_custom_target(libforestdb ALL
		DEPENDS ${PROJECT_BINARY_DIR}/db/forestdb/libforestdb${CMAKE_SHARED_LIBRARY_SUFFIX}
	)
	message(STATUS "Use shipped ForestDB: ${PROJECT_SOURCE_DIR}/db/forestdb")
	set (FORESTDB_LIBRARIES "${PROJECT_BINARY_DIR}/db/forestdb/libforestdb${CMAKE_SHARED_LIBRARY_SUFFIX}")
	set (FORESTDB_FOUND 1)
	add_dependencies(build_libs libforestdb)
endmacro(build_forestdb)
