#
# Try to find GLEW library and include path.
# Once done this will define
#
# GLEW_FOUND
# GLEW_INCLUDE_DIRS
# GLEW_LIBRARY
# 

#  Created on 2/6/2014 by Stephen Birarda
#
# Adapted from FindGLEW.cmake available in the nvidia-texture-tools repository
# (https://code.google.com/p/nvidia-texture-tools/source/browse/trunk/cmake/FindGLEW.cmake?r=96)

IF (WIN32)    
	FIND_PATH(GLEW_INCLUDE_DIRS GL/glew.h
    "${GLEW_ROOT_DIR}/include"
    "$ENV{GLEW_ROOT_DIR}/include"
	  "$ENV{HIFI_LIB_DIR}/glew/include"
	  DOC "The directory where GL/glew.h resides")
  if (CMAKE_CL_64)
    set(WIN_ARCH_DIR "x64")
  else()
    set(LINUX_ARCH_DIR "Win32")
  endif()
	
	FIND_LIBRARY(GLEW_LIBRARY
		NAMES glew GLEW glew32 glew32s
		PATHS
    "${GLEW_ROOT_DIR}/lib/Release/${WIN_ARCH_DIR}"
    "$ENV{GLEW_ROOT_DIR}/lib/Release/${WIN_ARCH_DIR}"
		"$ENV{HIFI_LIB_DIR}/glew/lib/Release/${WIN_ARCH_DIR}"
		DOC "The GLEW library")
ENDIF()

IF (GLEW_INCLUDE_PATH)
	SET( GLEW_FOUND 1 CACHE STRING "Set to 1 if GLEW is found, 0 otherwise")
ELSE (GLEW_INCLUDE_PATH)
	SET( GLEW_FOUND 0 CACHE STRING "Set to 1 if GLEW is found, 0 otherwise")
ENDIF (GLEW_INCLUDE_PATH)

MARK_AS_ADVANCED(GLEW_FOUND)