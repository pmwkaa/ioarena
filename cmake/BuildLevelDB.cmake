
macro(build_leveldb)
	set(LEVELDB_INCLUDE_DIRS ${PROJECT_SOURCE_DIR}/db/leveldb/include)
	set(LEVELDB_OPTS CFLAGS="${CMAKE_C_FLAGS}" LDFLAGS="${CMAKE_SHARED_LINKER_FLAGS}")
	separate_arguments(LEVELDB_OPTS)
	if (${PROJECT_BINARY_DIR} STREQUAL ${PROJECT_SOURCE_DIR})
		add_custom_command(
			OUTPUT ${PROJECT_BINARY_DIR}/db/leveldb/libleveldb${CMAKE_SHARED_LIBRARY_SUFFIX}
			COMMAND ${CMAKE_MAKE_PROGRAM} -j ${MAKE_NJOB} ${LEVELDB_OPTS} -C ${PROJECT_BINARY_DIR}/db/leveldb
			DEPENDS ${CMAKE_BINARY_DIR}/CMakeCache.txt
		)
	else()
		add_custom_command(
			OUTPUT ${PROJECT_BINARY_DIR}/db/leveldb/libleveldb${CMAKE_SHARED_LIBRARY_SUFFIX}
			COMMAND ${CMAKE_COMMAND} -E make_directory ${PROJECT_BINARY_DIR}/db/leveldb
			COMMAND ${CMAKE_COMMAND} -E copy_directory ${PROJECT_SOURCE_DIR}/db/leveldb ${PROJECT_BINARY_DIR}/db/leveldb
			COMMAND ${CMAKE_MAKE_PROGRAM} -j ${MAKE_NJOB} ${LEVELDB_OPTS} -C ${PROJECT_BINARY_DIR}/db/leveldb
			DEPENDS ${PROJECT_BINARY_DIR}/CMakeCache.txt
		)
	endif()
	add_custom_target(libleveldb ALL
		DEPENDS ${PROJECT_BINARY_DIR}/db/leveldb/libleveldb${CMAKE_SHARED_LIBRARY_SUFFIX}
	)
	message(STATUS "Use shipped LevelDB: ${PROJECT_SOURCE_DIR}/db/leveldb")
	set (LEVELDB_LIBRARIES "${PROJECT_BINARY_DIR}/db/leveldb/libleveldb${CMAKE_SHARED_LIBRARY_SUFFIX}")
	set (LEVELDB_FOUND 1)
	add_dependencies(build_libs libleveldb)
endmacro(build_leveldb)
