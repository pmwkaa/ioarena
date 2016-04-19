
macro(build_lmdb)
	set(LMDB_INCLUDE_DIRS ${PROJECT_SOURCE_DIR}/db/lmdb/libraries/liblmdb)
	if (${PROJECT_BINARY_DIR} STREQUAL ${PROJECT_SOURCE_DIR})
		add_custom_command(
			OUTPUT ${PROJECT_BINARY_DIR}/db/lmdb/libraries/liblmdb/liblmdb.so
			COMMAND ${CMAKE_MAKE_PROGRAM} -j ${MAKE_NJOB} -C ${PROJECT_BINARY_DIR}/db/lmdb/libraries/liblmdb liblmdb.so
			DEPENDS ${CMAKE_BINARY_DIR}/CMakeCache.txt
		)
	else()
		add_custom_command(
			OUTPUT ${PROJECT_BINARY_DIR}/db/lmdb/libraries/liblmdb/liblmdb.so
			COMMAND ${CMAKE_COMMAND} -E make_directory ${PROJECT_BINARY_DIR}/db/lmdb/libraries/liblmdb
			COMMAND ${CMAKE_COMMAND} -E copy_directory ${PROJECT_SOURCE_DIR}/db/lmdb/libraries/liblmdb ${PROJECT_BINARY_DIR}/db/lmdb/libraries/liblmdb
			COMMAND ${CMAKE_MAKE_PROGRAM} -j ${MAKE_NJOB} -C ${PROJECT_BINARY_DIR}/db/lmdb/libraries/liblmdb liblmdb.so
			DEPENDS ${PROJECT_BINARY_DIR}/CMakeCache.txt
		)
	endif()
	add_custom_target(liblmdb ALL
		DEPENDS ${PROJECT_BINARY_DIR}/db/lmdb/libraries/liblmdb/liblmdb.so
	)
	message(STATUS "Use shipped LMDB: ${PROJECT_SOURCE_DIR}/db/lmdb/libraries/liblmdb")
	set (LMDB_LIBRARIES "${PROJECT_BINARY_DIR}/db/lmdb/libraries/liblmdb/liblmdb.so")
	set (LMDB_FOUND 1)
	add_dependencies(build_libs liblmdb)
endmacro(build_lmdb)
