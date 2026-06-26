macro(build_iowow)
  if (${PROJECT_BINARY_DIR} STREQUAL ${PROJECT_SOURCE_DIR})
      set(IOWOW_FOUND 0)
      message( SEND_ERROR "IOWOW CAN'T BUILD IN SAME DIRECTORY WHERE PLACED SOURCES" )
  else()
    include(ExternalProject)
    include(GNUInstallDirs)
    ExternalProject_Add(
      iowow
      SOURCE_DIR "${PROJECT_SOURCE_DIR}/db/iowow"
      CONFIGURE_HANDLED_BY_BUILD ON
      BUILD_IN_SOURCE TRUE
      CONFIGURE_COMMAND ""
      BUILD_COMMAND ${CMAKE_COMMAND} -E chdir <SOURCE_DIR> ./build.sh  --install --prefix=${PROJECT_BINARY_DIR}/db/iowow -DIOWOW_BUILD_SHARED_LIBS=1
      INSTALL_COMMAND ""
      LOG_BUILD TRUE
      LOG_OUTPUT_ON_FAILURE TRUE
    )
  endif()
  message(STATUS "Use shipped IOWOW: ${PROJECT_SOURCE_DIR}/db/iowow")
  set(IOWOW_LIBRARIES "${PROJECT_BINARY_DIR}/db/iowow/${CMAKE_INSTALL_LIBDIR}/libiowow${CMAKE_SHARED_LIBRARY_SUFFIX}")
  set(IOWOW_INCLUDE_DIRS 	${PROJECT_BINARY_DIR}/db/iowow/include)
  set(IOWOW_FOUND 1)
  add_dependencies(build_libs iowow)
endmacro(build_iowow)
