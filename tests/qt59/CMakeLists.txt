
set(TARGET_NAME qt59)

if (WIN32)
  SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /ignore:4049 /ignore:4217")
endif()

setup_memory_debugger()

# This is not a testcase -- just set it up as a regular hifi project
setup_hifi_project(Gui)
set_target_properties(${TARGET_NAME} PROPERTIES FOLDER "Tests/manual-tests/")

# link in the shared libraries
link_hifi_libraries(shared networking)

package_libraries_for_deployment()
