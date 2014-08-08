# 
#  AutoMTC.cmake
# 
#  Created by Andrzej Kapolka on 12/31/13.
#  Copyright 2013 High Fidelity, Inc.
#
#  Distributed under the Apache License, Version 2.0.
#  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
# 

macro(AUTO_MTC)
  set(AUTOMTC_SRC ${TARGET_NAME}_automtc.cpp)
  
  file(GLOB INCLUDE_FILES src/*.h)
  
  add_custom_command(OUTPUT ${AUTOMTC_SRC} COMMAND mtc -o ${AUTOMTC_SRC} ${INCLUDE_FILES} DEPENDS mtc ${INCLUDE_FILES})
endmacro()
