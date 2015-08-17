
macro(build_leveldb)
    set(LEVELDB_INCLUDE_DIRS ${PROJECT_SOURCE_DIR}/db/leveldb/include)
	set(LEVELDB_OPTS
	    CFLAGS="${CMAKE_C_FLAGS}"
	    LDFLAGS="${CMAKE_SHARED_LINKER_FLAGS}")
	separate_arguments(LEVELDB_OPTS)
    if (${PROJECT_BINARY_DIR} STREQUAL ${PROJECT_SOURCE_DIR})
		add_custom_command(OUTPUT ${PROJECT_SOURCE_DIR}/db/leveldb/libleveldb.so
			WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/db/leveldb
			COMMAND $(MAKE) ${LEVELDB_OPTS}
			DEPENDS ${CMAKE_SOURCE_DIR}/CMakeCache.txt
		)
    else()
        add_custom_command(OUTPUT ${PROJECT_BINARY_DIR}/db/leveldb
            COMMAND ${CMAKE_COMMAND} -E make_directory ${PROJECT_BINARY_DIR}/db/leveldb
        )
		add_custom_command(OUTPUT ${PROJECT_BINARY_DIR}/db/leveldb/libleveldb.so
			WORKING_DIRECTORY ${PROJECT_BINARY_DIR}/db/leveldb
            COMMAND ${CMAKE_COMMAND} -E copy_directory ${PROJECT_SOURCE_DIR}/db/leveldb ${PROJECT_BINARY_DIR}/db/leveldb
			COMMAND $(MAKE) ${LEVELDB_OPTS}
            DEPENDS ${PROJECT_BINARY_DIR}/CMakeCache.txt ${PROJECT_BINARY_DIR}/db/leveldb
		)
    endif()
	add_custom_target(libleveldb ALL
		DEPENDS ${PROJECT_BINARY_DIR}/db/leveldb/libleveldb.so
	)
    message(STATUS "Use shipped LevelDB: ${PROJECT_SOURCE_DIR}/db/leveldb")
    set (LEVELDB_LIBRARIES "${PROJECT_BINARY_DIR}/db/leveldb/libleveldb.so")
    set (LEVELDB_FOUND 1)
    add_dependencies(build_libs libleveldb)
endmacro(build_leveldb)
