# 
#  Copyright 2015 High Fidelity, Inc.
#  Created by Leonardo Murillo on 2015/11/20
#
#  Distributed under the Apache License, Version 2.0.
#  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
# 
macro(TARGET_QUAZIP)
  find_library(QUAZIP_LIBRARY_RELEASE quazip5 PATHS ${VCPKG_INSTALL_ROOT}/lib NO_DEFAULT_PATH)
  find_library(QUAZIP_LIBRARY_DEBUG quazip5 PATHS ${VCPKG_INSTALL_ROOT}/debug/lib NO_DEFAULT_PATH)
  select_library_configurations(QUAZIP)
  target_link_libraries(${TARGET_NAME} ${QUAZIP_LIBRARIES})
endmacro()
