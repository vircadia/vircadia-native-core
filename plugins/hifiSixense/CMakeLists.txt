#
#  Created by Bradley Austin Davis on 2015/11/18
#  Copyright 2015 High Fidelity, Inc.
#
#  Distributed under the Apache License, Version 2.0.
#  See the accompanying file LICENSE or http:#www.apache.org/licenses/LICENSE-2.0.html
#

# FIXME if we want to properly re-enable this, we need to fix the mechanism for installing the 
# dependency dlls `msvcr100` and `msvcp100` WITHOUT using CMAKE_INSTALL_SYSTEM_RUNTIME_LIBS
# currently this will build the hifiSixense plugin, but unless the user has previously installed
# the msvcr100 runtime support, the plugin will not load.
if (NOT ANDROID)
  set(TARGET_NAME hifiSixense)
  setup_hifi_plugin(Script Qml Widgets)
  link_hifi_libraries(shared controllers ui plugins ui-plugins input-plugins)
  target_sixense()
endif ()
