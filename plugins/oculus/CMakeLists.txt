#
#  Created by Bradley Austin Davis on 2015/10/25
#  Copyright 2015 High Fidelity, Inc.
#
#  Distributed under the Apache License, Version 2.0.
#  See the accompanying file LICENSE or http:#www.apache.org/licenses/LICENSE-2.0.html
#

if (WIN32)

  # we're using static GLEW, so define GLEW_STATIC
  add_definitions(-DGLEW_STATIC)

  # if we were passed an Oculus App ID for entitlement checks, send that along
  if (DEFINED ENV{OCULUS_APP_ID})
    add_definitions(-DOCULUS_APP_ID="\\"$ENV{OCULUS_APP_ID}\\"")
  endif ()

  set(TARGET_NAME oculus)
  setup_hifi_plugin(Multimedia)
  link_hifi_libraries(
    shared gl gpu gpu-gl controllers ui
    plugins ui-plugins display-plugins input-plugins
    audio-client networking render-utils
  )
  include_hifi_library_headers(octree)

  add_dependency_external_projects(LibOVR)
  find_package(LibOVR REQUIRED)
  target_include_directories(${TARGET_NAME} PRIVATE ${LIBOVR_INCLUDE_DIRS})
  target_link_libraries(${TARGET_NAME} ${LIBOVR_LIBRARIES})
  target_link_libraries(${TARGET_NAME} Winmm.lib)

  add_dependency_external_projects(LibOVRPlatform)
  find_package(LibOVRPlatform REQUIRED)
  target_include_directories(${TARGET_NAME} PRIVATE ${LIBOVRPLATFORM_INCLUDE_DIRS})
  target_link_libraries(${TARGET_NAME} ${LIBOVRPLATFORM_LIBRARIES})
endif()
