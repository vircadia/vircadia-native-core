# 
#  FindLibOVR.cmake
# 
#  Try to find the LibOVR library to use the Oculus

#  Once done this will define
#
#  OPENVR_FOUND - system found LibOVR
#  OPENVR_INCLUDE_DIRS - the LibOVR include directory
#  OPENVR_LIBRARIES - Link this to use LibOVR
#
#  Distributed under the Apache License, Version 2.0.
#  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
# 

if (NOT ANDROID)
  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(OPENVR DEFAULT_MSG OPENVR_INCLUDE_DIRS OPENVR_LIBRARIES)
endif()

mark_as_advanced(OPENVR_INCLUDE_DIRS OPENVR_LIBRARIES OPENVR_SEARCH_DIRS)
