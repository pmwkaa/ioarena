
macro(build_rocksdb)
    set(ROCKSDB_INCLUDE_DIRS ${PROJECT_SOURCE_DIR}/db/rocksdb/include)
	set(ROCKSDB_OPTS
	    CFLAGS="${CMAKE_C_FLAGS}"
	    LDFLAGS="${CMAKE_SHARED_LINKER_FLAGS}")
	separate_arguments(ROCKSDB_OPTS)
    if (${PROJECT_BINARY_DIR} STREQUAL ${PROJECT_SOURCE_DIR})
		add_custom_command(OUTPUT ${PROJECT_SOURCE_DIR}/db/rocksdb/librocksdb.so
			WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/db/rocksdb
			COMMAND $(MAKE) ${ROCKSDB_OPTS}
			DEPENDS ${CMAKE_SOURCE_DIR}/CMakeCache.txt
		)
    else()
        add_custom_command(OUTPUT ${PROJECT_BINARY_DIR}/db/rocksdb
            COMMAND ${CMAKE_COMMAND} -E make_directory ${PROJECT_BINARY_DIR}/db/rocksdb
        )
		add_custom_command(OUTPUT ${PROJECT_BINARY_DIR}/db/rocksdb/librocksdb.so
			WORKING_DIRECTORY ${PROJECT_BINARY_DIR}/db/rocksdb
            COMMAND ${CMAKE_COMMAND} -E copy_directory ${PROJECT_SOURCE_DIR}/db/rocksdb ${PROJECT_BINARY_DIR}/db/rocksdb
			COMMAND $(MAKE) ${ROCKSDB_OPTS}
            DEPENDS ${PROJECT_BINARY_DIR}/CMakeCache.txt ${PROJECT_BINARY_DIR}/db/rocksdb
		)
    endif()
	add_custom_target(librocksdb ALL
		DEPENDS ${PROJECT_BINARY_DIR}/db/rocksdb/librocksdb.so
	)
    message(STATUS "Use shipped RocksDB: ${PROJECT_SOURCE_DIR}/db/rocksdb")
    set (ROCKSDB_LIBRARIES "${PROJECT_BINARY_DIR}/db/rocksdb/librocksdb.so")
    set (ROCKSDB_FOUND 1)
    add_dependencies(build_libs librocksdb)
endmacro(build_rocksdb)
