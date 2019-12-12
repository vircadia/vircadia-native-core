# add the manual test directories
file(GLOB TEST_SUBDIRS RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}/*")

if (APPLE)
    set(CMAKE_MACOSX_BUNDLE TRUE)
endif()

foreach(DIR ${TEST_SUBDIRS})
  if((IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/${DIR}") AND (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${DIR}/CMakeLists.txt"))
    set(TEST_PROJ_NAME ${DIR})
    add_subdirectory(${DIR})
  endif()
endforeach()

if (APPLE)
    unset(CMAKE_MACOSX_BUNDLE)
endif()
