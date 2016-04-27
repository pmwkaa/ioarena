
macro(build_wiredtiger)
	set(WIREDTIGER_INCLUDE_DIRS ${PROJECT_BINARY_DIR}/db/wiredtiger)
	set(WIREDTIGER_OPTS CFLAGS="${CMAKE_C_FLAGS}" LDFLAGS="${CMAKE_SHARED_LINKER_FLAGS}")
	separate_arguments(WIREDTIGER_OPTS)
	if (${PROJECT_BINARY_DIR} STREQUAL ${PROJECT_SOURCE_DIR})
		add_custom_command(
			OUTPUT ${PROJECT_BINARY_DIR}/db/wiredtiger/.libs/libwiredtiger${CMAKE_SHARED_LIBRARY_SUFFIX}
			WORKING_DIRECTORY ${PROJECT_BINARY_DIR}/db/wiredtiger
			COMMAND ./autogen.sh
			COMMAND ./configure
			COMMAND ${CMAKE_MAKE_PROGRAM} -j ${MAKE_NJOB} ${WIREDTIGER_OPTS} -C ${PROJECT_BINARY_DIR}/db/wiredtiger
			DEPENDS ${CMAKE_BINARY_DIR}/CMakeCache.txt
		)
	else()
		add_custom_command(
			OUTPUT ${PROJECT_BINARY_DIR}/db/wiredtiger/.libs/libwiredtiger${CMAKE_SHARED_LIBRARY_SUFFIX}
			WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/db/wiredtiger
			COMMAND ./autogen.sh
			COMMAND ./configure
			COMMAND ${CMAKE_COMMAND} -E make_directory ${PROJECT_BINARY_DIR}/db/wiredtiger
			COMMAND ${CMAKE_COMMAND} -E copy_directory ${PROJECT_SOURCE_DIR}/db/wiredtiger ${PROJECT_BINARY_DIR}/db/wiredtiger
			COMMAND ${CMAKE_MAKE_PROGRAM} -j ${MAKE_NJOB} ${WIREDTIGER_OPTS} -C ${PROJECT_BINARY_DIR}/db/wiredtiger
			DEPENDS ${PROJECT_BINARY_DIR}/CMakeCache.txt
		)
	endif()
	add_custom_target(libwiredtiger ALL
		DEPENDS ${PROJECT_BINARY_DIR}/db/wiredtiger/.libs/libwiredtiger${CMAKE_SHARED_LIBRARY_SUFFIX}
	)
	message(STATUS "Use shipped WiredTiger: ${PROJECT_SOURCE_DIR}/db/wiredtiger")
	set (WIREDTIGER_LIBRARIES "${PROJECT_BINARY_DIR}/db/wiredtiger/.libs/libwiredtiger${CMAKE_SHARED_LIBRARY_SUFFIX}")
	set (WIREDTIGER_FOUND 1)
	add_dependencies(build_libs libwiredtiger)
endmacro(build_wiredtiger)
