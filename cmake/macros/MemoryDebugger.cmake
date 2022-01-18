#
#  MemoryDebugger.cmake
#
#  Copyright 2015 High Fidelity, Inc.
#
#  Distributed under the Apache License, Version 2.0.
#  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
#

macro(SETUP_MEMORY_DEBUGGER)
if ("$ENV{VIRCADIA_MEMORY_DEBUGGING}")
  if (VIRCADIA_THREAD_DEBUGGING)
    message(FATAL_ERROR "Thread debugging and memory debugging can't be enabled at the same time." )
  endif()

  SET( VIRCADIA_MEMORY_DEBUGGING true )
endif ()

if (VIRCADIA_MEMORY_DEBUGGING)
  if (CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-omit-frame-pointer -fsanitize=undefined -fsanitize=address -fsanitize-recover=address")
    SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=undefined -fsanitize=address -fsanitize-recover=address")
    SET(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=undefined -fsanitize=address -fsanitize-recover=address")
    if (UNIX)
      # Only supported on Linux and OSX
      # https://clang.llvm.org/docs/LeakSanitizer.html
      SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=leak")
      SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=leak")
      SET(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=leak")
    endif()
  elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    # for gcc
    # For some reason, using -fstack-protector results in this error:
    # usr/bin/ld: ../../libraries/audio/libaudio.so: undefined reference to `FIR_1x4_AVX512(float*, float*, float*, float*, float*, float (*) [64], int)'
    # The '-DSTACK_PROTECTOR' argument below disables the usage of this function in the code. This should be fine as it only works on the latest Intel hardware,
    # and is an optimization that should make no functional difference.

    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=undefined -fsanitize=address -fsanitize=leak -U_FORTIFY_SOURCE -DSTACK_PROTECTOR -fstack-protector-strong -fno-omit-frame-pointer")
    SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=undefined -fsanitize=address -fsanitize=leak ")
    SET(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=undefined -fsanitize=address -fsanitize=leak")
  elseif(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    # https://docs.microsoft.com/en-us/cpp/sanitizers/asan?view=msvc-160
    # Supported experimentally starting from VS2019 v16.4, and officially from v16.9.
    # UBSan and leak detection don't seem to be implemented yet.
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /fsanitize=address /Zi")
  else()
    message(FATAL_ERROR "Memory debugging is not supported on this compiler.")
  endif()
endif ()
endmacro(SETUP_MEMORY_DEBUGGER)
