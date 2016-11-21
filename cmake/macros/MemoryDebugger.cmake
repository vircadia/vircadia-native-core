#
#  MemoryDebugger.cmake
#
#  Copyright 2015 High Fidelity, Inc.
#
#  Distributed under the Apache License, Version 2.0.
#  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
#

macro(SETUP_MEMORY_DEBUGGER)
if (DEFINED ENV{HIFI_MEMORY_DEBUGGING})
  SET( HIFI_MEMORY_DEBUGGING true )
endif ()

if (HIFI_MEMORY_DEBUGGING)
  if (UNIX)
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address -U_FORTIFY_SOURCE -fno-stack-protector -fno-omit-frame-pointer")
    SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libasan -static-libstdc++ -fsanitize=address")
    SET(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libasan -static-libstdc++ -fsanitize=address")
  endif (UNIX)
endif ()
endmacro(SETUP_MEMORY_DEBUGGER)
