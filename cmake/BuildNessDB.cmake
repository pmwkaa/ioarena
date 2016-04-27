
macro(build_nessdb)
    set(NESSDB_INCLUDE_DIRS ${PROJECT_SOURCE_DIR}/db/nessdb/db/)
    set(NESSDB_INCLUDE_DIRS ${PROJECT_SOURCE_DIR}/db/nessdb/include/)
	set(NESSDB_OPTS "")
	separate_arguments(NESSDB_OPTS)
    if (${PROJECT_BINARY_DIR} STREQUAL ${PROJECT_SOURCE_DIR})
		add_custom_command(OUTPUT ${PROJECT_SOURCE_DIR}/db/nessdb/libnessdb${CMAKE_SHARED_LIBRARY_SUFFIX}
			WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/db/nessdb
			COMMAND $(MAKE) ${NESSDB_OPTS}
			DEPENDS ${CMAKE_SOURCE_DIR}/CMakeCache.txt
		)
    else()
        add_custom_command(OUTPUT ${PROJECT_BINARY_DIR}/db/nessdb
            COMMAND ${CMAKE_COMMAND} -E make_directory ${PROJECT_BINARY_DIR}/db/nessdb
        )
		add_custom_command(OUTPUT ${PROJECT_BINARY_DIR}/db/nessdb/libnessdb${CMAKE_SHARED_LIBRARY_SUFFIX}
			WORKING_DIRECTORY ${PROJECT_BINARY_DIR}/db/nessdb
            COMMAND ${CMAKE_COMMAND} -E copy_directory ${PROJECT_SOURCE_DIR}/db/nessdb ${PROJECT_BINARY_DIR}/db/nessdb
			COMMAND $(MAKE) ${NESSDB_OPTS}
            DEPENDS ${PROJECT_BINARY_DIR}/CMakeCache.txt ${PROJECT_BINARY_DIR}/db/nessdb
		)
    endif()
	add_custom_target(libnessdb ALL
		DEPENDS ${PROJECT_BINARY_DIR}/db/nessdb//libnessdb${CMAKE_SHARED_LIBRARY_SUFFIX}
	)
    message(STATUS "Use shipped nessDB: ${PROJECT_SOURCE_DIR}/db/nessdb")
    set (NESSDB_LIBRARIES "${PROJECT_BINARY_DIR}/db/nessdb/libnessdb${CMAKE_SHARED_LIBRARY_SUFFIX}")
    set (NESSDB_FOUND 1)
    add_dependencies(build_libs libnessdb)
endmacro(build_nessdb)
