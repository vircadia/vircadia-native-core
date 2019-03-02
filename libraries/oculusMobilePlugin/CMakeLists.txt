#
#  Created by Bradley Austin Davis on 2018/11/15
#  Copyright 2013-2018 High Fidelity, Inc.
#
#  Distributed under the Apache License, Version 2.0.
#  See the accompanying file LICENSE or http:#www.apache.org/licenses/LICENSE-2.0.html
#

if (ANDROID)
  set(TARGET_NAME oculusMobilePlugin)
  setup_hifi_library(AndroidExtras Multimedia)

  # if we were passed an Oculus App ID for entitlement checks, send that along
  if (DEFINED ENV{OCULUS_APP_ID})
    target_compile_definitions(${TARGET_NAME} -DOCULUS_APP_ID="$ENV{OCULUS_APP_ID}")
  endif ()

  link_hifi_libraries(
    shared task gl shaders gpu controllers ui qml
    plugins ui-plugins display-plugins input-plugins
    audio-client networking render-utils
    render graphics
    oculusMobile 
    ${PLATFORM_GL_BACKEND}
  )
  
  include_hifi_library_headers(octree)
  target_oculus_mobile()
endif()
