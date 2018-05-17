macro(build_iowow)
  if (${PROJECT_BINARY_DIR} STREQUAL ${PROJECT_SOURCE_DIR})
      set(IOWOW_FOUND 0)
      message( SEND_ERROR "IOWOW CAN'T BUILD IN SAME DIRECTORY WHERE PLACED SOURCES" )
  else()
    include(ExternalProject)
    ExternalProject_Add(
      iowow
      SOURCE_DIR "${PROJECT_SOURCE_DIR}/db/iowow"
      BINARY_DIR "${PROJECT_BINARY_DIR}/db/iowow"
      CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=${PROJECT_BINARY_DIR}/db/iowow -DBUILD_SHARED_LIBS=ON -DBUILD_EXAMPLES=OFF -DCMAKE_BUILD_TYPE=Release
    )
  endif()
  message(STATUS "Use shipped IOWOW: ${PROJECT_SOURCE_DIR}/db/iowow")
  set(IOWOW_LIBRARIES "${PROJECT_BINARY_DIR}/db/iowow/lib/libiowow${CMAKE_SHARED_LIBRARY_SUFFIX}")
  set(IOWOW_INCLUDE_DIRS 	${PROJECT_BINARY_DIR}/db/iowow/include)
  set(IOWOW_FOUND 1)
  add_dependencies(build_libs iowow)
endmacro(build_iowow)