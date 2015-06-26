
# Turn on testing (so that add_test works)
enable_testing()

# add the test directories
file(GLOB TEST_SUBDIRS RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}/*")
list(REMOVE_ITEM TEST_SUBDIRS "CMakeFiles")
foreach(DIR ${TEST_SUBDIRS})
    if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/${DIR}")
    	set(TEST_PROJ_NAME ${DIR})
        add_subdirectory(${DIR}) 	# link in the subdir (this reinvokes cmake on the cmakelists.txt file, with its
    endif()							# own variable scope copied from this scope (the parent scope)).
endforeach()

file(GLOB SHARED_TEST_HEADER_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.h" "${CMAKE_CURRENT_SOURCE_DIR}/*.hpp")

add_custom_target("test-extensions"
	SOURCES "${SHARED_TEST_HEADER_FILES}")
list(APPEND ALL_TEST_TARGETS "test-extensions")
set_target_properties("test-extensions" PROPERTIES FOLDER "Tests")

message(STATUS "ALL_TEST_TARGETS = ${ALL_TEST_TARGETS}")

# Create the all-tests build target.
# The dependency list (ALL_TEST_TARGETS) is generated from setup_hifi_testcase invocations in the CMakeLists.txt
# files in the test subdirs. Since variables normally do *not* persist into parent scope, we use a hack:
#
#	list(APPEND ALL_TEST_TARGETS ${targets_to_add...})	# appends to a local list var (copied from parent scope)
#	set (ALL_TEST_TARGETS "${ALL_TEST_TARGETS}" PARENT_SCOPE)	# copies this back to parent scope
#
add_custom_target("all-tests" ALL 
	COMMAND ctest .
	DEPENDS "${ALL_TEST_TARGETS}")
set_target_properties("all-tests" PROPERTIES FOLDER "hidden/test-targets")

# Note: we also do some funky stuff with macros (SETUP_TESTCASE_DEPENDENCIES is redefined in *each* CMakeLists.txt
# file, and then invoked in SetupHifiTestCase.cmake) -- which is necessary since the dependencies must be re-linked
# for each target (instead of just one)