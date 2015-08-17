
macro(build_forestdb)
    set(FORESTDB_INCLUDE_DIRS ${PROJECT_SOURCE_DIR}/db/forestdb/include/libforestdb)
	set(FORESTDB_OPTS
	    CFLAGS="${CMAKE_C_FLAGS}"
	    LDFLAGS="${CMAKE_SHARED_LINKER_FLAGS}")
	separate_arguments(FORESTDB_OPTS)
    if (${PROJECT_BINARY_DIR} STREQUAL ${PROJECT_SOURCE_DIR})
		add_custom_command(OUTPUT ${PROJECT_SOURCE_DIR}/db/forestdb/libforestdb.so
			WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/db/forestdb
			COMMAND cmake .
			COMMAND $(MAKE) ${FORESTDB_OPTS}
			DEPENDS ${CMAKE_SOURCE_DIR}/CMakeCache.txt
		)
    else()
        add_custom_command(OUTPUT ${PROJECT_BINARY_DIR}/db/forestdb
            COMMAND ${CMAKE_COMMAND} -E make_directory ${PROJECT_BINARY_DIR}/db/forestdb
        )
		add_custom_command(OUTPUT ${PROJECT_BINARY_DIR}/db/forestdb/libforestdb.so
			WORKING_DIRECTORY ${PROJECT_BINARY_DIR}/db/forestdb
            COMMAND ${CMAKE_COMMAND} -E copy_directory ${PROJECT_SOURCE_DIR}/db/forestdb ${PROJECT_BINARY_DIR}/db/forestdb
			COMMAND cmake .
			COMMAND $(MAKE) ${FORESTDB_OPTS}
            DEPENDS ${PROJECT_BINARY_DIR}/CMakeCache.txt ${PROJECT_BINARY_DIR}/db/forestdb
		)
    endif()
	add_custom_target(libforestdb ALL
		DEPENDS ${PROJECT_BINARY_DIR}/db/forestdb/libforestdb.so
	)
    message(STATUS "Use shipped ForestDB: ${PROJECT_SOURCE_DIR}/db/forestdb")
    set (FORESTDB_LIBRARIES "${PROJECT_BINARY_DIR}/db/forestdb/libforestdb.so")
    set (FORESTDB_FOUND 1)
    add_dependencies(build_libs libforestdb)
endmacro(build_forestdb)
