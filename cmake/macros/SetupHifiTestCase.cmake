# 
#  SetupHifiTestCase.cmake
# 
#  Copyright 2015 High Fidelity, Inc.
#
#  Distributed under the Apache License, Version 2.0.
#  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
# 

# Sets up a hifi testcase using QtTest.
# Can be called with arguments; like setup_hifi_project, the arguments correspond to qt modules, so call it
# via setup_hifi_testcase(Script Network Qml) to build the testcase with Qt5Script, Qt5Network, and Qt5Qml linked,
# for example.

# One special quirk of this is that because we are creating multiple testcase targets (instead of just one),
# any dependencies and other setup that the testcase has must be declared in a macro, and setup_hifi_testcase()
# must be called *after* this declaration, not before it.

# Here's a full example:
# tests/my-foo-test/CMakeLists.txt:
# 
# # Declare testcase dependencies
# macro (setup_hifi_testcase)
#   bunch
#   of
#   custom
#   dependencies...
#
#   link_hifi_libraries(shared networking etc)
#
#   copy_dlls_beside_windows_executable()
# endmacro()
#
# setup_hifi_testcase(Network etc)
#
# Additionally, all .cpp files in the src dir (eg tests/my-foo-test/src) must:
# - Contain exactly one test class (any supporting code must be either external or inline in a .hpp file)
#   - Be built against QtTestLib (test class should be a QObject, with test methods defined as private slots)
#   - Contain a QTEST_MAIN declaration at the end of the file (or at least be a standalone executable)
#
# All other testing infrastructure is generated automatically.
#

macro(SETUP_HIFI_TESTCASE)
  if (NOT DEFINED TEST_PROJ_NAME)
    message(SEND_ERROR "Missing TEST_PROJ_NAME (setup_hifi_testcase was called incorrectly?)")
  elseif (NOT COMMAND SETUP_TESTCASE_DEPENDENCIES)
    message(SEND_ERROR "Missing testcase dependencies declaration (SETUP_TESTCASE_DEPENDENCIES)")
  elseif (DEFINED ${TEST_PROJ_NAME}_BUILT)
    message(WARNING "testcase \"" ${TEST_PROJ_NAME} "\" was already built")
  else ()
    set(${TEST_PROJ_NAME}_BUILT 1)

    file(GLOB TEST_PROJ_SRC_FILES src/*)
    file(GLOB TEST_PROJ_SRC_SUBDIRS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}/src ${CMAKE_CURRENT_SOURCE_DIR}/src/*)

    foreach (DIR ${TEST_PROJ_SRC_SUBDIRS})
      if (IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/src/${DIR}")
        file(GLOB DIR_CONTENTS "src/${DIR}/*")
        set(TEST_PROJ_SRC_FILES ${TEST_PROJ_SRC_FILES} "${DIR_CONTENTS}")
      endif()
    endforeach()

    # Find test classes to build into test executables.
    # Warn about any .cpp files that are *not* test classes (*Test[s].cpp), since those files will not be used.
    foreach (SRC_FILE ${TEST_PROJ_SRC_FILES})   
      string(REGEX MATCH ".+Tests?\\.cpp$"   TEST_CPP_FILE ${SRC_FILE})
      string(REGEX MATCH ".+\\.cpp$"     NON_TEST_CPP_FILE ${SRC_FILE}) 
      if (TEST_CPP_FILE)
        list(APPEND TEST_CASE_FILES ${TEST_CPP_FILE})
      elseif (NON_TEST_CPP_FILE)
        message(WARNING "ignoring .cpp file (not a test class -- this will not be linked or compiled!): " ${NON_TEST_CPP_FILE})
      endif ()
    endforeach ()

    if (TEST_CASE_FILES)
      set(TEST_PROJ_TARGETS "")
      
      # Add each test class executable (duplicates functionality in SetupHifiProject.cmake)
      # The resulting targets will be buried inside of hidden/test-executables so that they don't clutter up
      # the project view in Xcode, Visual Studio, etc.
      foreach (TEST_FILE ${TEST_CASE_FILES})
        get_filename_component(TEST_NAME ${TEST_FILE} NAME_WE)
        set(TARGET_NAME ${TEST_PROJ_NAME}-${TEST_NAME})
            
        project(${TARGET_NAME}) 
      
        # grab the implemenation and header files
        set(TARGET_SRCS ${TEST_FILE})  # only one source / .cpp file (the test class)
      
        add_executable(${TARGET_NAME} ${TEST_FILE})
        add_test(${TARGET_NAME}-test  ${TARGET_NAME})
        set_target_properties(${TARGET_NAME} PROPERTIES 
          EXCLUDE_FROM_DEFAULT_BUILD TRUE
          EXCLUDE_FROM_ALL TRUE)


        list (APPEND ${TEST_PROJ_NAME}_TARGETS ${TARGET_NAME})
        #list (APPEND ALL_TEST_TARGETS ${TARGET_NAME})

        set(${TARGET_NAME}_DEPENDENCY_QT_MODULES ${ARGN})
  
        list(APPEND ${TARGET_NAME}_DEPENDENCY_QT_MODULES Core Test)
        
        # find these Qt modules and link them to our own target
        find_package(Qt5 COMPONENTS ${${TARGET_NAME}_DEPENDENCY_QT_MODULES} REQUIRED)
      
        foreach(QT_MODULE ${${TARGET_NAME}_DEPENDENCY_QT_MODULES})
          target_link_libraries(${TARGET_NAME} Qt5::${QT_MODULE})
        endforeach()
        target_link_libraries(${TARGET_NAME} ${CMAKE_THREAD_LIBS_INIT})
      
        set_target_properties(${TARGET_NAME} PROPERTIES FOLDER "hidden/test-executables")
        
        # handle testcase-specific dependencies (this a macro that should be defined in the cmakelists.txt file   in each tests subdir)
        SETUP_TESTCASE_DEPENDENCIES()
        target_glm()
        
      endforeach ()
      
      set(TEST_TARGET ${TEST_PROJ_NAME}-tests)
      
      # Add a dummy target so that the project files are visible.
      # This target will also build + run the other test targets using ctest when built.

      add_custom_target(${TEST_TARGET}
        COMMAND ctest .
        SOURCES ${TEST_PROJ_SRC_FILES}    # display source files under the testcase target
        DEPENDS ${${TEST_PROJ_NAME}_TARGETS})
      set_target_properties(${TEST_TARGET} PROPERTIES
        EXCLUDE_FROM_DEFAULT_BUILD TRUE
        EXCLUDE_FROM_ALL TRUE)

      set_target_properties(${TEST_TARGET} PROPERTIES FOLDER "Tests")

      list (APPEND ALL_TEST_TARGETS ${TEST_TARGET})
      set(ALL_TEST_TARGETS "${ALL_TEST_TARGETS}" PARENT_SCOPE)
    else ()
      message(WARNING "No testcases in " ${TEST_PROJ_NAME})
    endif ()
  endif ()
endmacro(SETUP_HIFI_TESTCASE)