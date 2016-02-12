
cmake_minimum_required(VERSION 2.8)

project(ioarena)
message (STATUS "IOARENA: embedded storage benchmarking.")
execute_process (COMMAND getconf _NPROCESSORS_ONLN
	OUTPUT_VARIABLE NB_PROCESSOR OUTPUT_STRIP_TRAILING_WHITESPACE)
math (EXPR MAKE_NJOB "((${NB_PROCESSOR}) + 1) * 3 / 2")
message (STATUS "")

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH}
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=c99 -Wall -Wextra -g")
if ("${CMAKE_BUILD_TYPE}" STREQUAL "Release")
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O2")
	set(CMAKE_C_FLAGS "${CMAKE_CXx_FLAGS} -O2")
elseif("${CMAKE_BUILD_TYPE}" STREQUAL "Debug")
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O0")
	set(CMAKE_C_FLAGS "${CMAKE_CXX_FLAGS} -O0")
endif()

# LY: rpath hint for running with libraries that
#     we had compiled (or found), but avoids mixing.
set(CMAKE_SKIP_BUILD_RPATH FALSE)

add_custom_target(build_libs)
set(ia_libraries "")

# LevelDB
option(ENABLE_LEVELDB "Enable LevelDB" OFF)
if (ENABLE_LEVELDB)
	find_package(LevelDB QUIET)
	if (NOT LEVELDB_FOUND)
		include(BuildLevelDB)
		build_leveldb()
	endif (NOT LEVELDB_FOUND)
	set(HAVE_LEVELDB 1)
	set(ia_libraries ${ia_libraries} ${LEVELDB_LIBRARIES})
	include_directories(${LEVELDB_INCLUDE_DIRS})
endif(ENABLE_LEVELDB)

# RocksDB
option(ENABLE_ROCKSDB "Enable RocksDB" OFF)
if (ENABLE_ROCKSDB)
	find_package(RocksDB QUIET)
	if (NOT ROCKSDB_FOUND)
		include(BuildRocksDB)
		build_rocksdb()
	endif (NOT ROCKSDB_FOUND)
	set(HAVE_ROCKSDB 1)
	set(ia_libraries ${ia_libraries} ${ROCKSDB_LIBRARIES})
	include_directories(${ROCKSDB_INCLUDE_DIRS})
endif(ENABLE_ROCKSDB)

# Sophia
option(ENABLE_SOPHIA  "Enable Sophia" OFF)
if (ENABLE_SOPHIA)
	find_package(Sophia QUIET)
	if (NOT SOPHIA_FOUND)
		include(BuildSophia)
		build_sophia()
	endif (NOT SOPHIA_FOUND)
	set(HAVE_SOPHIA 1)
	set(ia_libraries ${ia_libraries} ${SOPHIA_LIBRARIES})
	include_directories(${SOPHIA_INCLUDE_DIRS})
endif(ENABLE_SOPHIA)

# LMDB
option(ENABLE_LMDB "Enable LMDB" OFF)
if (ENABLE_LMDB)
	find_package(LMDB QUIET)
	if (NOT LMDB_FOUND)
		include(BuildLMDB)
		build_lmdb()
	endif (NOT LMDB_FOUND)
	set(HAVE_LMDB 1)
	set(ia_libraries ${ia_libraries} ${LMDB_LIBRARIES})
	include_directories(${LMDB_INCLUDE_DIRS})
endif(ENABLE_LMDB)

# LMDBx
option(ENABLE_MDBX "Enable MDBX" OFF)
if (ENABLE_MDBX)
	include(BuildMDBX)
	build_mdbx()
	set(HAVE_MDBX 1)
	set(ia_libraries ${ia_libraries} ${MDBX_LIBRARIES})
	include_directories(${MDBX_INCLUDE_DIRS})
endif(ENABLE_MDBX)

# WiredTiger
option(ENABLE_WIREDTIGER "Enable WiredTiger" OFF)
if (ENABLE_WIREDTIGER)
	find_package(WiredTiger QUIET)
	if (NOT WIREDTIGER_FOUND)
		include(BuildWiredTiger)
		build_wiredtiger()
	endif (NOT WIREDTIGER_FOUND)
	set(HAVE_WIREDTIGER 1)
	set(ia_libraries ${ia_libraries} ${WIREDTIGER_LIBRARIES})
	include_directories(${WIREDTIGER_INCLUDE_DIRS})
endif(ENABLE_WIREDTIGER)

# ForestDB
option(ENABLE_FORESTDB "Enable ForestDB" OFF)
if (ENABLE_FORESTDB)
	find_package(ForestDB QUIET)
	if (NOT FORESTDB_FOUND)
		include(BuildForestDB)
		build_forestdb()
	endif (NOT FORESTDB_FOUND)
	set(HAVE_FORESTDB 1)
	set(ia_libraries ${ia_libraries} ${FORESTDB_LIBRARIES})
	include_directories(${FORESTDB_INCLUDE_DIRS})
endif(ENABLE_FORESTDB)

# NessDB
option(ENABLE_NESSDB "Enable nessDB" OFF)
if (ENABLE_NESSDB)
	find_package(nessDB QUIET)
	if (NOT NESSDB_FOUND)
		include(BuildNessDB)
		build_nessdb()
	endif (NOT NESSDB_FOUND)
	set(HAVE_NESSDB 1)
	set(ia_libraries ${ia_libraries} ${NESSDB_LIBRARIES})
	include_directories(${NESSDB_INCLUDE_DIRS})
endif(ENABLE_NESSDB)

message (STATUS "")
message (STATUS "IOARENA: embedded storage benchmarking.")

message (STATUS "")
message (STATUS "ENABLE_LEVELDB:    ${ENABLE_LEVELDB}")
message (STATUS "ENABLE_ROCKSDB:    ${ENABLE_ROCKSDB}")
message (STATUS "ENABLE_SOPHIA:     ${ENABLE_SOPHIA}")
message (STATUS "ENABLE_WIREDTIGER: ${ENABLE_WIREDTIGER}")
message (STATUS "ENABLE_FORESTDB:   ${ENABLE_FORESTDB}")
message (STATUS "ENABLE_LMDB:       ${ENABLE_LMDB}")
message (STATUS "ENABLE_MDBX:       ${ENABLE_MDBX}")
message (STATUS "ENABLE_NESSDB:     ${ENABLE_NESSDB}")
message (STATUS "")

#message (STATUS "NB_PROCESSOR: ${NB_PROCESSOR}")
#message (STATUS "MAKE_NJOB: ${MAKE_NJOB}")
#message (STATUS "")

add_subdirectory(src)
