#
#  Created by Clément Brisset on 12/14/16
#  Copyright 2016 High Fidelity, Inc.
#
#  Distributed under the Apache License, Version 2.0.
#  See the accompanying file LICENSE or http:#www.apache.org/licenses/LICENSE-2.0.html
#

set(TARGET_NAME steamClient)
setup_hifi_plugin(Gui)
link_hifi_libraries(plugins)
target_steamworks()
