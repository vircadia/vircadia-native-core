#
#  Created by Brad Hefta-Gaub on 7/10/2016
#  Copyright 2016 High Fidelity, Inc.
#
#  Distributed under the Apache License, Version 2.0.
#  See the accompanying file LICENSE or http:#www.apache.org/licenses/LICENSE-2.0.html
#

set(TARGET_NAME hifiCodec)
setup_hifi_client_server_plugin()
link_hifi_libraries(shared audio plugins)
target_hifiAudioCodec()
if (BUILD_SERVER)
  install_beside_console()
endif ()
