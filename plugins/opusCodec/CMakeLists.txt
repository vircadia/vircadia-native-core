#
#  Created by Michael Bailey on 12/20/2019
#  Copyright 2019 Michael Bailey
#
#  Distributed under the Apache License, Version 2.0.
#  See the accompanying file LICENSE or http:#www.apache.org/licenses/LICENSE-2.0.html
#

set(TARGET_NAME opusCodec)
setup_hifi_client_server_plugin()
link_hifi_libraries(shared audio plugins)
target_opus()

if (BUILD_SERVER)
    install_beside_console()
endif ()
