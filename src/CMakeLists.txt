set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Werror")

set(ia_binary ioarena)
set(ia_src
    ia_debug.c
    ia_rusage.c
    ia_benchmark.c
    ia_histogram.c
    ia_dummy.c
    ia_config.c
    ia_kv.c
    ia_log.c
    ia.c
    ia_main.c)

include_directories("${PROJECT_SOURCE_DIR}/src")
include_directories("${PROJECT_BINARY_DIR}/src")

if (HAVE_LEVELDB)
	set(ia_src ${ia_src} ia_leveldb.c)
endif (HAVE_LEVELDB)

if (HAVE_ROCKSDB)
	set(ia_src ${ia_src} ia_rocksdb.c)
endif (HAVE_ROCKSDB)

if (HAVE_WIREDTIGER)
	set(ia_src ${ia_src} ia_wiredtiger.c)
endif (HAVE_WIREDTIGER)

if (HAVE_SOPHIA)
	set(ia_src ${ia_src} ia_sophia.c)
endif (HAVE_SOPHIA)

if (HAVE_FORESTDB)
	set(ia_src ${ia_src} ia_forestdb.c)
endif (HAVE_FORESTDB)

if (HAVE_LMDB)
	set(ia_src ${ia_src} ia_lmdb.c)
endif (HAVE_LMDB)

if (HAVE_MDBX)
	set(ia_src ${ia_src} ia_mdbx.c)
endif (HAVE_MDBX)

if (HAVE_NESSDB)
	set(ia_src ${ia_src} ia_nessdb.c)
endif (HAVE_NESSDB)

add_executable(${ia_binary} ${ia_src})
add_dependencies(${ia_binary} build_libs)

target_link_libraries(${ia_binary} ${ia_libraries} m rt pthread)

configure_file(
	"ia_build.h.cmake"
	"ia_build.h"
)
