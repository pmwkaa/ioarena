
macro(build_mdbx)
	set(MDBX_INCLUDE_DIRS ${PROJECT_SOURCE_DIR}/db/mdbx)
	if (${PROJECT_BINARY_DIR} STREQUAL ${PROJECT_SOURCE_DIR})
		add_custom_command(
			OUTPUT ${PROJECT_BINARY_DIR}/db/mdbx/libmdbx.so
			COMMAND ${CMAKE_MAKE_PROGRAM} -j ${MAKE_NJOB} -C ${PROJECT_BINARY_DIR}/db/mdbx libmdbx.so
			DEPENDS ${CMAKE_BINARY_DIR}/CMakeCache.txt
		)
	else()
		add_custom_command(
			OUTPUT ${PROJECT_BINARY_DIR}/db/mdbx/libmdbx.so
			COMMAND ${CMAKE_COMMAND} -E make_directory ${PROJECT_BINARY_DIR}/db/mdbx
			COMMAND ${CMAKE_COMMAND} -E copy_directory ${PROJECT_SOURCE_DIR}/db/mdbx ${PROJECT_BINARY_DIR}/db/mdbx
			COMMAND ${CMAKE_MAKE_PROGRAM} -j ${MAKE_NJOB} -C ${PROJECT_BINARY_DIR}/db/mdbx libmdbx.so
			DEPENDS ${PROJECT_BINARY_DIR}/CMakeCache.txt
		)
	endif()
	add_custom_target(libmdbx ALL
		DEPENDS ${PROJECT_BINARY_DIR}/db/mdbx/libmdbx.so
	)
	message(STATUS "Use shipped MDBX: ${PROJECT_SOURCE_DIR}/db/mdbx")
	set (MDBX_LIBRARIES "${PROJECT_BINARY_DIR}/db/mdbx/libmdbx.so")
	set (MDBX_FOUND 1)
	add_dependencies(build_libs libmdbx)
endmacro(build_mdbx)
