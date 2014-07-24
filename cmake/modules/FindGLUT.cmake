# 
#  FindGLUT.cmake
# 
#  Try to find GLUT library and include path.
#  Once done this will define
#
#  GLUT_FOUND
#  GLUT_INCLUDE_DIRS
#  GLUT_LIBRARIES
#
#  Created on 2/6/2014 by Stephen Birarda
#  Copyright 2014 High Fidelity, Inc.
# 
#  Adapted from FindGLUT.cmake available in tlorach's OpenGLText Repository
#  https://raw.github.com/tlorach/OpenGLText/master/cmake/FindGLUT.cmake
# 
#  Distributed under the Apache License, Version 2.0.
#  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
# 

include("${MACRO_DIR}/HifiLibrarySearchHints.cmake")
hifi_library_search_hints("GLUT" "freeglut")

if (WIN32)
  set(GLUT_HINT_DIRS "${GLUT_HINT_DIRS} ${OPENGL_INCLUDE_DIR}")s
  
  find_path(GLUT_INCLUDE_DIRS GL/glut.h PATH_SUFFIXES include HINTS ${GLUT_HINT_DIRS})
  find_library(GLUT_LIBRARY freeglut PATH_SUFFIXES lib HINTS ${GLUT_HINT_DIRS})
else ()
  find_path(GLUT_INCLUDE_DIRS GL/glut.h PATH_SUFFIXES include HINTS ${GLUT_HINT_DIRS})
  find_library(GLUT_LIBRARY glut PATH_SUFFIXES lib HINTS ${GLUT_HINT_DIRS})
endif ()

include(FindPackageHandleStandardArgs)

set(GLUT_LIBRARIES "${GLUT_LIBRARY}" "${XMU_LIBRARY}" "${XI_LIBRARY}")

if (UNIX)
  find_library(XI_LIBRARY Xi PATH_SUFFIXES lib HINTS ${GLUT_HINT_DIRS})
  find_library(XMU_LIBRARY Xmu PATH_SUFFIXES lib HINTS ${GLUT_HINT_DIRS})
  
  find_package_handle_standard_args(GLUT DEFAULT_MSG GLUT_INCLUDE_DIRS GLUT_LIBRARIES XI_LIBRARY XMU_LIBRARY)
else ()
  find_package_handle_standard_args(GLUT DEFAULT_MSG GLUT_INCLUDE_DIRS GLUT_LIBRARIES)
endif ()

mark_as_advanced(GLUT_INCLUDE_DIRS GLUT_LIBRARIES GLUT_LIBRARY XI_LIBRARY XMU_LIBRARY)