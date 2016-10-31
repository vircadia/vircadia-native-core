#
#  Created by Bradley Austin Davis on 2015/11/18
#  Copyright 2015 High Fidelity, Inc.
#
#  Distributed under the Apache License, Version 2.0.
#  See the accompanying file LICENSE or http:#www.apache.org/licenses/LICENSE-2.0.html
#

if (NOT ANDROID)
  set(TARGET_NAME hifiSixense)
  setup_hifi_plugin(Script Qml Widgets)
  link_hifi_libraries(shared controllers ui plugins ui-plugins input-plugins)
  target_sixense()
endif ()
