
macro(build_sophia)
	set(SOPHIA_INCLUDE_DIRS ${PROJECT_SOURCE_DIR}/db/sophia/sophia/sophia)
	set(SOPHIA_OPTS CFLAGS="${CMAKE_C_FLAGS}" LDFLAGS="${CMAKE_SHARED_LINKER_FLAGS}")
	separate_arguments(SOPHIA_OPTS)
	if (${PROJECT_BINARY_DIR} STREQUAL ${PROJECT_SOURCE_DIR})
		add_custom_command(
			OUTPUT ${PROJECT_BINARY_DIR}/db/sophia/libsophia${CMAKE_SHARED_LIBRARY_SUFFIX}
			COMMAND ${CMAKE_MAKE_PROGRAM} -j ${MAKE_NJOB} ${SOPHIA_OPTS} -C ${PROJECT_BINARY_DIR}/db/sophia libsophia${CMAKE_SHARED_LIBRARY_SUFFIX}
			DEPENDS ${CMAKE_BINARY_DIR}/CMakeCache.txt
		)
	else()
		add_custom_command(
			OUTPUT ${PROJECT_BINARY_DIR}/db/sophia/libsophia${CMAKE_SHARED_LIBRARY_SUFFIX}
			COMMAND ${CMAKE_COMMAND} -E make_directory ${PROJECT_BINARY_DIR}/db/sophia
			COMMAND ${CMAKE_COMMAND} -E copy_directory ${PROJECT_SOURCE_DIR}/db/sophia ${PROJECT_BINARY_DIR}/db/sophia
			COMMAND ${CMAKE_MAKE_PROGRAM} -j ${MAKE_NJOB} ${SOPHIA_OPTS} -C ${PROJECT_BINARY_DIR}/db/sophia libsophia${CMAKE_SHARED_LIBRARY_SUFFIX}
			DEPENDS ${PROJECT_BINARY_DIR}/CMakeCache.txt
		)
	endif()
	add_custom_target(libsophia ALL
		DEPENDS ${PROJECT_BINARY_DIR}/db/sophia/libsophia${CMAKE_SHARED_LIBRARY_SUFFIX}
	)
	message(STATUS "Use shipped Sophia: ${PROJECT_SOURCE_DIR}/db/sophia")
	set (SOPHIA_LIBRARIES "${PROJECT_BINARY_DIR}/db/sophia/libsophia${CMAKE_SHARED_LIBRARY_SUFFIX}")
	set (SOPHIA_FOUND 1)
	add_dependencies(build_libs libsophia)
endmacro(build_sophia)
