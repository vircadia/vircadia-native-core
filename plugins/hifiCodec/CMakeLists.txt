#
#  Created by Brad Hefta-Gaub on 7/10/2016
#  Copyright 2016 High Fidelity, Inc.
#
#  Distributed under the Apache License, Version 2.0.
#  See the accompanying file LICENSE or http:#www.apache.org/licenses/LICENSE-2.0.html
#

set(TARGET_NAME hifiCodec)
setup_hifi_client_server_plugin()
link_hifi_libraries(audio shared plugins)
add_dependency_external_projects(hifiAudioCodec)
target_include_directories(${TARGET_NAME} PRIVATE ${HIFIAUDIOCODEC_INCLUDE_DIRS})
target_link_libraries(${TARGET_NAME} ${HIFIAUDIOCODEC_LIBRARIES})
install_beside_console()

