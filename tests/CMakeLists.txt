# add the test directories
file(GLOB TEST_SUBDIRS RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}/*")
foreach(DIR ${TEST_SUBDIRS})
    if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/${DIR}")
        add_subdirectory(${DIR})
    endif()
endforeach()