#
#  FindNSIGHT.cmake
# 
#  Try to find NSIGHT NvToolsExt library and include path.
#  Once done this will define
#
#  NSIGHT_FOUND
#  NSIGHT_INCLUDE_DIRS
#  NSIGHT_LIBRARIES
# 
#  Created on 10/27/2014 by Sam Gateau
#  Copyright 2014 High Fidelity, Inc.
#
#  Distributed under the Apache License, Version 2.0.
#  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
# 

if (WIN32)
    
    if ("${CMAKE_SIZEOF_VOID_P}" EQUAL "8")
        set(ARCH_DIR "x64")
        set(ARCH_NAME "64")
    else()
        set(ARCH_DIR "Win32")
        set(ARCH_NAME "32")
    endif()


	find_path(NSIGHT_INCLUDE_DIRS
		NAMES
			nvToolsExt.h
		PATH_SUFFIXES
			include
		PATHS
		 	"C:/Program Files/NVIDIA Corporation/NvToolsExt")
  
	find_library(NSIGHT_LIBRARY_RELEASE "nvToolsExt${ARCH_NAME}_1"
	 	PATH_SUFFIXES
	 		"lib/${ARCH_DIR}" "lib"
	 	PATHS
	 		"C:/Program Files/NVIDIA Corporation/NvToolsExt")
	find_library(NSIGHT_LIBRARY_DEBUG "nvToolsExt${ARCH_NAME}_1"
	 	PATH_SUFFIXES
	 		"lib/${ARCH_DIR}" "lib"
	 	PATHS
	 		"C:/Program Files/NVIDIA Corporation/NvToolsExt")

  add_paths_to_fixup_libs("C:/Program Files/NVIDIA Corporation/NvToolsExt/bin/${ARCH_DIR}")
  include(SelectLibraryConfigurations)
  select_library_configurations(NSIGHT)
endif ()

set(NSIGHT_LIBRARIES "${NSIGHT_LIBRARY}")

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(NSIGHT DEFAULT_MSG NSIGHT_INCLUDE_DIRS NSIGHT_LIBRARIES)

mark_as_advanced(NSIGHT_INCLUDE_DIRS NSIGHT_LIBRARIES NSIGHT_SEARCH_DIRS)

