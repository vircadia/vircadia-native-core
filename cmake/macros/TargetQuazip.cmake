# 
#  Copyright 2015 High Fidelity, Inc.
#  Created by Leonardo Murillo on 2015/11/20
#
#  Distributed under the Apache License, Version 2.0.
#  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
# 
macro(TARGET_QUAZIP)
  add_dependency_external_projects(quazip)
  find_package(QuaZip REQUIRED)
  target_include_directories(${TARGET_NAME} PUBLIC ${QUAZIP_INCLUDE_DIRS})
  target_link_libraries(${TARGET_NAME} ${QUAZIP_LIBRARIES})
  if (WIN32)
    add_paths_to_fixup_libs(${QUAZIP_DLL_PATH})
  endif ()
endmacro()