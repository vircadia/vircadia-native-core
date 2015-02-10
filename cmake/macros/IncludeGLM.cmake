# 
#  IncludeGLM.cmake
# 
#  Copyright 2013 High Fidelity, Inc.
#
#  Distributed under the Apache License, Version 2.0.
#  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
# 

macro(INCLUDE_GLM)
	find_package(GLM REQUIRED)
	include_directories(SYSTEM "${GLM_INCLUDE_DIRS}")
endmacro(INCLUDE_GLM)