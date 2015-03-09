# 
#  FindLibOVR.cmake
# 
#  Try to find the LibOVR library to use the Oculus
#
#  You must provide a LIBOVR_ROOT_DIR which contains Lib and Include directories
#
#  Once done this will define
#
#  LIBOVR_FOUND - system found LibOVR
#  LIBOVR_INCLUDE_DIRS - the LibOVR include directory
#  LIBOVR_LIBRARIES - Link this to use LibOVR
#
#  Created on 5/9/2013 by Stephen Birarda
#  Copyright 2013 High Fidelity, Inc.
#
#  Distributed under the Apache License, Version 2.0.
#  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
# 


if (NOT ANDROID)

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(LIBOVR DEFAULT_MSG LIBOVR_INCLUDE_DIRS LIBOVR_LIBRARIES)

else (NOT ANDROID)  
  set(_VRLIB_JNI_DIR "VRLib/jni")
  set(_VRLIB_LIBS_DIR "VRLib/obj/local/armeabi-v7a")
  
  find_path(LIBOVR_VRLIB_DIR VRLib.vcxproj PATH_SUFFIXES VRLib HINTS ${LIBOVR_SEARCH_DIRS})
  
  find_path(LIBOVR_INCLUDE_DIRS OVR.h PATH_SUFFIXES ${_VRLIB_JNI_DIR}/LibOVR/Include HINTS ${LIBOVR_SEARCH_DIRS})
  find_path(LIBOVR_SRC_DIR OVR_CAPI.h PATH_SUFFIXES ${_VRLIB_JNI_DIR}/LibOVR/Src HINTS ${LIBOVR_SEARCH_DIRS})
  
  find_path(MINIZIP_DIR minizip.c PATH_SUFFIXES ${_VRLIB_JNI_DIR}/3rdParty/minizip HINTS ${LIBOVR_SEARCH_DIRS})
  find_path(JNI_DIR VrCommon.h PATH_SUFFIXES ${_VRLIB_JNI_DIR} HINTS ${LIBOVR_SEARCH_DIRS})
  
  find_library(LIBOVR_LIBRARY_RELEASE NAMES oculus PATH_SUFFIXES ${_VRLIB_LIBS_DIR} HINTS ${LIBOVR_SEARCH_DIRS})
  find_library(TURBOJPEG_LIBRARY NAMES jpeg PATH_SUFFIXES 3rdParty/turbojpeg HINTS ${LIBOVR_SEARCH_DIRS})
endif (NOT ANDROID)

mark_as_advanced(LIBOVR_INCLUDE_DIRS LIBOVR_LIBRARIES LIBOVR_SEARCH_DIRS)
