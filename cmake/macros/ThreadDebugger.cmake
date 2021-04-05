#
#  MemoryDebugger.cmake
#
#  Copyright 2021 Vircadia Contributors
#
#  Distributed under the Apache License, Version 2.0.
#  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
#

macro(SETUP_THREAD_DEBUGGER)
if ("$ENV{VIRCADIA_THREAD_DEBUGGING}")
  if (VIRCADIA_MEMORY_DEBUGGING )
    message(FATAL_ERROR "Thread debugging and memory debugging can't be enabled at the same time." )
  endif ()

  SET(VIRCADIA_THREAD_DEBUGGING true)
endif ()

if (VIRCADIA_THREAD_DEBUGGING)
  if (UNIX)
    if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        # for clang on Linux
        SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=thread -fno-omit-frame-pointer")
        SET(CMAKE_EXE_LINKER_FLAGS "-fsanitize=thread ${CMAKE_EXE_LINKER_FLAGS}")
        SET(CMAKE_SHARED_LINKER_FLAGS "-fsanitize=thread ${CMAKE_EXE_LINKER_FLAGS}")
    else ()
        # for gcc on Linux
        SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=thread -fno-omit-frame-pointer")
        SET(CMAKE_EXE_LINKER_FLAGS " -fsanitize=thread ${CMAKE_EXE_LINKER_FLAGS}")
        SET(CMAKE_SHARED_LINKER_FLAGS "-fsanitize=thread ${CMAKE_EXE_LINKER_FLAGS}")
    endif()
  else()
    message(FATAL_ERROR "Thread debugging is not supported on this platform.")
  endif()
endif ()
endmacro(SETUP_THREAD_DEBUGGER)
