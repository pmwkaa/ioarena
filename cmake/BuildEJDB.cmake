
macro(build_ejdb)
	set(EJDB_INCLUDE_DIRS 	${PROJECT_SOURCE_DIR}/db/ejdb/src/ejdb
				${PROJECT_SOURCE_DIR}/db/ejdb/src/bson
				${PROJECT_SOURCE_DIR}/db/ejdb/src/tcutil
				${PROJECT_BINARY_DIR}/db/ejdb/src/generated
	)
	set(EJDB_OPTS CFLAGS="${CMAKE_C_FLAGS}" LDFLAGS="${CMAKE_SHARED_LINKER_FLAGS}")
	separate_arguments(EJDB_OPTS)
	if (${PROJECT_BINARY_DIR} STREQUAL ${PROJECT_SOURCE_DIR})
		set(EJDB_FOUND 0)
		message( SEND_ERROR "EJDB CAN'T BUILD IN SAME DIRECTORY WHERE PLACED SOURCES" )
	else()
		include(ExternalProject)
		ExternalProject_Add(
			ejdb
#			GIT_REPOSITORY "https://github.com/Softmotions/ejdb.git"
#			GIT_TAG "master"
#			UPDATE_COMMAND ""
#			PATCH_COMMAND ""
			SOURCE_DIR "${PROJECT_SOURCE_DIR}/db/ejdb"
			BINARY_DIR "${PROJECT_BINARY_DIR}/db/ejdb"
#			CMAKE_ARGS -DBuildShared=ON -DBuildExamples=OFF -DCMAKE_INSTALL_PREFIX=${GLOBAL_OUTPUT_PATH}/humblelogging
			CMAKE_ARGS -DBUILD_SHARED_LIBS=ON -DBUILD_SAMPLES=ON -DCMAKE_BUILD_TYPE=Release #Debug|Release|RelWithDebInfo
			INSTALL_COMMAND cmake -E echo "Skipping install step."
#			TEST_COMMAND ""
			)
	endif()
	message(STATUS "Use shipped EJDB: ${PROJECT_SOURCE_DIR}/db/ejdb")
	set(EJDB_LIBRARIES "${PROJECT_BINARY_DIR}/db/ejdb/src/libejdb${CMAKE_SHARED_LIBRARY_SUFFIX}")
	set(EJDB_FOUND 1)
	message(STATUS "EJDB LIBRARY: ${PROJECT_SOURCE_DIR}/db/ejdb/src/libejdb${CMAKE_SHARED_LIBRARY_SUFFIX}")
	#include_directories(${EJDB_INCLUDE_DIRS})
	add_dependencies(build_libs ejdb)
endmacro(build_ejdb)
