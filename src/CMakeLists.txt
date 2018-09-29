set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Werror")

set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
find_package(Threads REQUIRED)

set(ia_binary ioarena)
set(ia_src
  ia_rusage.c
  ia_benchmark.c
  ia_histogram.c
  ia_config.c
  ia_kv.c
  ia_log.c
  ia.c
  ia_time.c
  ia_main.c
  drivers/ia_debug.c
  drivers/ia_dummy.c
  )

if(THREADS_HAVE_PTHREAD_ARG)
  set_property(TARGET ${ia_binary} PROPERTY COMPILE_OPTIONS "-pthread")
  set_property(TARGET ${ia_binary} PROPERTY INTERFACE_COMPILE_OPTIONS "-pthread")
endif()

include_directories("${PROJECT_SOURCE_DIR}/src")
include_directories("${PROJECT_BINARY_DIR}/src")

if (HAVE_LEVELDB)
  set(ia_src ${ia_src} drivers/ia_leveldb.c)
endif (HAVE_LEVELDB)

if (HAVE_ROCKSDB)
  set(ia_src ${ia_src} drivers/ia_rocksdb.c)
endif (HAVE_ROCKSDB)

if (HAVE_WIREDTIGER)
  set(ia_src ${ia_src} drivers/ia_wiredtiger.c)
endif (HAVE_WIREDTIGER)

if (HAVE_SOPHIA)
  set(ia_src ${ia_src} drivers/ia_sophia.c)
endif (HAVE_SOPHIA)

if (HAVE_FORESTDB)
  set(ia_src ${ia_src} drivers/ia_forestdb.c)
endif (HAVE_FORESTDB)

if (HAVE_LMDB)
  set(ia_src ${ia_src} drivers/ia_lmdb.c)
endif (HAVE_LMDB)

if (HAVE_MDBX)
  set(ia_src ${ia_src} drivers/ia_mdbx.c)
endif (HAVE_MDBX)

if (HAVE_SQLITE3)
  set(ia_src ${ia_src} drivers/ia_sqlite3.c)
endif (HAVE_SQLITE3)

if (HAVE_EJDB)
  set(ia_src ${ia_src} drivers/ia_ejdb.c)
endif (HAVE_EJDB)

if (HAVE_UNQLITE)
  set(ia_src ${ia_src} drivers/ia_unqlite.c)
endif (HAVE_UNQLITE)

if (HAVE_UPSCALEDB)
  set(ia_src ${ia_src} drivers/ia_upscaledb.c)
endif (HAVE_UPSCALEDB)

if (HAVE_VEDISDB)
  add_library(vedisDB SHARED ../db/vedis/vedis.c)
  set_property(TARGET vedisDB APPEND_STRING PROPERTY COMPILE_FLAGS " -DVEDIS_ENABLE_THREADS -Wno-error -std=gnu89")
  target_link_libraries(vedisDB "${CMAKE_THREAD_LIBS_INIT}")

  set(ia_src ${ia_src} drivers/ia_vedisdb.c)
  set(ia_libraries ${ia_libraries} vedisDB)
endif (HAVE_VEDISDB)

if (HAVE_IOWOW)
  set(ia_src ${ia_src} drivers/ia_iowow.c)
endif (HAVE_IOWOW)

add_executable(${ia_binary} ${ia_src})
add_dependencies(${ia_binary} build_libs)

target_link_libraries(${ia_binary} ${ia_libraries} m "${CMAKE_THREAD_LIBS_INIT}")
if (NOT APPLE)
  target_link_libraries(${ia_binary} rt)
endif(NOT APPLE)

configure_file("ia_build.h.in" "ia_build.h")
