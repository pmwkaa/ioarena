
macro(build_lmdb)
    set(LMDB_INCLUDE_DIRS ${PROJECT_SOURCE_DIR}/db/lmdb/libraries/liblmdb)
	set(LMDB_OPTS "")
	separate_arguments(LMDB_OPTS)
    if (${PROJECT_BINARY_DIR} STREQUAL ${PROJECT_SOURCE_DIR})
		add_custom_command(OUTPUT ${PROJECT_SOURCE_DIR}/db/lmdb/libraries/liblmdb/liblmdb.so
			WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/db/lmdb/libraries/liblmdb/
			COMMAND $(MAKE) ${LMDB_OPTS}
			DEPENDS ${CMAKE_SOURCE_DIR}/CMakeCache.txt
		)
    else()
        add_custom_command(OUTPUT ${PROJECT_BINARY_DIR}/db/lmdb/libraries/liblmdb
            COMMAND ${CMAKE_COMMAND} -E make_directory ${PROJECT_BINARY_DIR}/db/lmdb/libraries/liblmdb
        )
		add_custom_command(OUTPUT ${PROJECT_BINARY_DIR}/db/lmdb/libraries/liblmdb/liblmdb.so
			WORKING_DIRECTORY ${PROJECT_BINARY_DIR}/db/lmdb/libraries/liblmdb
            COMMAND ${CMAKE_COMMAND} -E copy_directory ${PROJECT_SOURCE_DIR}/db/lmdb/libraries/liblmdb ${PROJECT_BINARY_DIR}/db/lmdb/libraries/liblmdb
			COMMAND $(MAKE) ${LMDB_OPTS}
            DEPENDS ${PROJECT_BINARY_DIR}/CMakeCache.txt ${PROJECT_BINARY_DIR}/db/lmdb/libraries/liblmdb
		)
    endif()
	add_custom_target(liblmdb ALL
		DEPENDS ${PROJECT_BINARY_DIR}/db/lmdb/libraries/liblmdb/liblmdb.so
	)
    message(STATUS "Use shipped LMDB: ${PROJECT_SOURCE_DIR}/db/lmdb")
    set (LMDB_LIBRARIES "${PROJECT_BINARY_DIR}/db/lmdb/libraries/liblmdb/liblmdb.so")
    set (LMDB_FOUND 1)
    add_dependencies(build_libs liblmdb)
endmacro(build_lmdb)
