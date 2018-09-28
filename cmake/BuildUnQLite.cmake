
macro(build_unqlite)
  set(UNQLITE_INCLUDE_DIRS ${PROJECT_BINARY_DIR}/db/unqlite)
  add_custom_command(
    OUTPUT ${PROJECT_BINARY_DIR}/db/unqlite/mkdir
    COMMAND ${CMAKE_COMMAND} -E make_directory ${PROJECT_BINARY_DIR}/db/unqlite
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${PROJECT_SOURCE_DIR}/db/unqlite ${PROJECT_BINARY_DIR}/db/unqlite
    COMMAND ${CMAKE_COMMAND} -E touch ${PROJECT_BINARY_DIR}/db/unqlite/mkdir
    )
  add_custom_command(
    OUTPUT ${PROJECT_BINARY_DIR}/db/unqlite/libunqlite${CMAKE_STATIC_LIBRARY_SUFFIX}
    WORKING_DIRECTORY ${PROJECT_BINARY_DIR}/db/unqlite
    COMMAND ${CMAKE_COMMAND} ${PROJECT_SOURCE_DIR}/db/unqlite
    COMMAND ${CMAKE_MAKE_PROGRAM} -j ${MAKE_NJOB}
    DEPENDS ${PROJECT_BINARY_DIR}/CMakeCache.txt ${PROJECT_BINARY_DIR}/db/unqlite/mkdir
    )
  add_custom_target(libunqlite ALL
    DEPENDS ${PROJECT_BINARY_DIR}/db/unqlite/libunqlite${CMAKE_STATIC_LIBRARY_SUFFIX}
    )
  message(STATUS "Use shipped UnQLite: ${PROJECT_SOURCE_DIR}/db/unqlite")
  set (UNQLITE_LIBRARIES "${PROJECT_BINARY_DIR}/db/unqlite/libunqlite${CMAKE_STATIC_LIBRARY_SUFFIX}")
  set (UNQLITE_FOUND 1)
  add_dependencies(build_libs libunqlite)
endmacro(build_unqlite)
