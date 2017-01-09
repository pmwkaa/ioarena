
macro(build_rocksdb)
	set(ROCKSDB_INCLUDE_DIRS ${PROJECT_SOURCE_DIR}/db/rocksdb/include)
	set(ROCKSDB_OPTS CFLAGS="${CMAKE_C_FLAGS}" LDFLAGS="${CMAKE_SHARED_LINKER_FLAGS}")
	separate_arguments(ROCKSDB_OPTS)
	if (${PROJECT_BINARY_DIR} STREQUAL ${PROJECT_SOURCE_DIR})
		add_custom_command(
			OUTPUT ${PROJECT_BINARY_DIR}/db/rocksdb/librocksdb${CMAKE_SHARED_LIBRARY_SUFFIX}
			COMMAND ${CMAKE_MAKE_PROGRAM} -j ${MAKE_NJOB} ${ROCKSDB_OPTS} -C ${PROJECT_BINARY_DIR}/db/rocksdb shared_lib
			DEPENDS ${CMAKE_BINARY_DIR}/CMakeCache.txt
		)
	else()
		add_custom_command(
			OUTPUT ${PROJECT_BINARY_DIR}/db/rocksdb/librocksdb${CMAKE_SHARED_LIBRARY_SUFFIX}
			COMMAND ${CMAKE_COMMAND} -E make_directory ${PROJECT_BINARY_DIR}/db/rocksdb
			COMMAND ${CMAKE_COMMAND} -E copy_directory ${PROJECT_SOURCE_DIR}/db/rocksdb ${PROJECT_BINARY_DIR}/db/rocksdb
			COMMAND ${CMAKE_MAKE_PROGRAM} -j ${MAKE_NJOB} ${ROCKSDB_OPTS} -C ${PROJECT_BINARY_DIR}/db/rocksdb shared_lib
			DEPENDS ${PROJECT_BINARY_DIR}/CMakeCache.txt
		)
	endif()
	add_custom_target(librocksdb ALL
		DEPENDS ${PROJECT_BINARY_DIR}/db/rocksdb/librocksdb${CMAKE_SHARED_LIBRARY_SUFFIX}
	)
	message(STATUS "Use shipped RocksDB: ${PROJECT_SOURCE_DIR}/db/rocksdb")
	set (ROCKSDB_LIBRARIES "${PROJECT_BINARY_DIR}/db/rocksdb/librocksdb${CMAKE_SHARED_LIBRARY_SUFFIX}" bz2 z lz4 snappy)
	set (ROCKSDB_FOUND 1)
	add_dependencies(build_libs librocksdb)
endmacro(build_rocksdb)
