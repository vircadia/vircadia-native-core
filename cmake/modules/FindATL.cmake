# 
#  FindATL.cmake
# 
#  Try to find the ATL library needed to use the LibOVR
#
#  Once done this will define
#
#  ATL_FOUND - system found ATL
#  ATL_LIBRARIES - Link this to use ATL
#
#  Created on 8/13/2013 by Stephen Birarda
#  Copyright 2014 High Fidelity, Inc.
#
#  Distributed under the Apache License, Version 2.0.
#  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
# 

if (WIN32)
	find_library(ATL_LIBRARY_RELEASE atl PATH_SUFFIXES "7600.16385.1/lib/ATL/i386" HINTS "C:\\WinDDK")
  find_library(ATL_LIBRARY_DEBUG atld PATH_SUFFIXES "7600.16385.1/lib/ATL/i386" HINTS "C:\\WinDDK")
  
  include(SelectLibraryConfigurations)
  select_library_configurations(ATL)
endif ()

set(ATL_LIBRARIES "${ATL_LIBRARY}")

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(ATL DEFAULT_MSG ATL_LIBRARIES)