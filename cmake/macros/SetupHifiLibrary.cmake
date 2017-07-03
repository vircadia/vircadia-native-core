#
#  SetupHifiLibrary.cmake
#
#  Copyright 2013 High Fidelity, Inc.
#
#  Distributed under the Apache License, Version 2.0.
#  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
#

macro(SETUP_HIFI_LIBRARY)

  project(${TARGET_NAME})

  # grab the implementation and header files
  file(GLOB_RECURSE LIB_SRCS "src/*.h" "src/*.cpp" "src/*.c")
  list(APPEND ${TARGET_NAME}_SRCS ${LIB_SRCS})

  # add compiler flags to AVX source files
  file(GLOB_RECURSE AVX_SRCS "src/avx/*.cpp" "src/avx/*.c")
  foreach(SRC ${AVX_SRCS})
    if (WIN32)
      set_source_files_properties(${SRC} PROPERTIES COMPILE_FLAGS /arch:AVX)
    elseif (APPLE OR UNIX)
      set_source_files_properties(${SRC} PROPERTIES COMPILE_FLAGS -mavx)
    endif()
  endforeach()

  # add compiler flags to AVX2 source files
  file(GLOB_RECURSE AVX2_SRCS "src/avx2/*.cpp" "src/avx2/*.c")
  foreach(SRC ${AVX2_SRCS})
    if (WIN32)
      set_source_files_properties(${SRC} PROPERTIES COMPILE_FLAGS /arch:AVX2)
    elseif (APPLE OR UNIX)
      set_source_files_properties(${SRC} PROPERTIES COMPILE_FLAGS "-mavx2 -mfma")
    endif()
  endforeach()

  # add compiler flags to AVX512 source files, if supported by compiler
  include(CheckCXXCompilerFlag)
  file(GLOB_RECURSE AVX512_SRCS "src/avx512/*.cpp" "src/avx512/*.c")
  foreach(SRC ${AVX512_SRCS})
    if (WIN32)
      check_cxx_compiler_flag("/arch:AVX512" COMPILER_SUPPORTS_AVX512)
      if (COMPILER_SUPPORTS_AVX512)
        set_source_files_properties(${SRC} PROPERTIES COMPILE_FLAGS /arch:AVX512)
      endif()
    elseif (APPLE OR UNIX)
      check_cxx_compiler_flag("-mavx512f" COMPILER_SUPPORTS_AVX512)
      if (COMPILER_SUPPORTS_AVX512)
        set_source_files_properties(${SRC} PROPERTIES COMPILE_FLAGS -mavx512f)
      endif()
    endif()
  endforeach()

  setup_memory_debugger()

  # create a library and set the property so it can be referenced later
  if (${${TARGET_NAME}_SHARED})
    add_library(${TARGET_NAME} SHARED ${LIB_SRCS} ${AUTOSCRIBE_SHADER_LIB_SRC} ${QT_RESOURCES_FILE})
  else ()
    add_library(${TARGET_NAME} ${LIB_SRCS} ${AUTOSCRIBE_SHADER_LIB_SRC} ${QT_RESOURCES_FILE})
  endif ()

  set(${TARGET_NAME}_DEPENDENCY_QT_MODULES ${ARGN})
  list(APPEND ${TARGET_NAME}_DEPENDENCY_QT_MODULES Core)

  # find these Qt modules and link them to our own target
  find_package(Qt5 COMPONENTS ${${TARGET_NAME}_DEPENDENCY_QT_MODULES} REQUIRED)

  foreach(QT_MODULE ${${TARGET_NAME}_DEPENDENCY_QT_MODULES})
    target_link_libraries(${TARGET_NAME} Qt5::${QT_MODULE})
  endforeach()

  # Don't make scribed shaders or QT resource files cumulative
  set(AUTOSCRIBE_SHADER_LIB_SRC "")
  set(QT_RESOURCES_FILE "")

  target_glm()

  set_target_properties(${TARGET_NAME} PROPERTIES FOLDER "Libraries")

endmacro(SETUP_HIFI_LIBRARY)
