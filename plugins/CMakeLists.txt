#
#  Created by Bradley Austin Davis on 2015/10/25
#  Copyright 2015 High Fidelity, Inc.
#
#  Distributed under the Apache License, Version 2.0.
#  See the accompanying file LICENSE or http:#www.apache.org/licenses/LICENSE-2.0.html
#

# add the plugin directories
file(GLOB PLUGIN_SUBDIRS RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}/*")
list(REMOVE_ITEM PLUGIN_SUBDIRS "CMakeFiles")
set(CMAKE_BUILD_TYPE "Release")
# client-side plugins
if (NOT SERVER_ONLY AND NOT ANDROID)
  if (NOT CMAKE_SYSTEM_PROCESSOR STREQUAL "aarch64")
    set(DIR "oculus")
    add_subdirectory(${DIR})
    set(DIR "openvr")
    add_subdirectory(${DIR})
    set(DIR "oculusLegacy")
    add_subdirectory(${DIR})
  endif()

  set(DIR "hifiSdl2")
  add_subdirectory(${DIR})

  if (USE_SIXENSE)
    set(DIR "hifiSixense")
    add_subdirectory(${DIR})
  endif()

  set(DIR "hifiSpacemouse")
  add_subdirectory(${DIR})
  set(DIR "hifiNeuron")
  add_subdirectory(${DIR})
  set(DIR "hifiKinect")
  add_subdirectory(${DIR})

  if (NOT CMAKE_SYSTEM_PROCESSOR STREQUAL "aarch64")
    set(DIR "steamClient")
    add_subdirectory(${DIR})
  endif()

  set(DIR "hifiLeapMotion")
  add_subdirectory(${DIR})

  if (WIN32)
    set(DIR "hifiOsc")
    add_subdirectory(${DIR})
  endif()

endif()

# server-side plugins
set(DIR "pcmCodec")
add_subdirectory(${DIR})
set(DIR "opusCodec")
add_subdirectory(${DIR})

if (NOT CMAKE_SYSTEM_PROCESSOR STREQUAL "aarch64")
  set(DIR "hifiCodec")
  add_subdirectory(${DIR})
endif()

# example plugins
set(DIR "JSAPIExample")
add_subdirectory(${DIR})
