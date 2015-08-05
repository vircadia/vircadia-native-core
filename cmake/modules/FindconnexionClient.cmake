#
#  FindconnexionClient.cmake
# 
#  Once done this will define
#
#  3DCONNEXIONCLIENT_INCLUDE_DIRS
# 
#  Created on 10/06/2015 by Marcel Verhagen
#  Copyright 2015 High Fidelity, Inc.
# 
#  Distributed under the Apache License, Version 2.0.
#  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
# 

# setup hints for 3DCONNEXIONCLIENT search
include("${MACRO_DIR}/HifiLibrarySearchHints.cmake")
hifi_library_search_hints("connexionclient")

if (APPLE)
	find_library(3DconnexionClient 3DconnexionClient)
	if(EXISTS ${3DconnexionClient})
		set(CONNEXIONCLIENT_FOUND true)
		set(CONNEXIONCLIENT_INCLUDE_DIR ${3DconnexionClient})
		set(CONNEXIONCLIENT_LIBRARY ${3DconnexionClient})
		set_target_properties(${TARGET_NAME} PROPERTIES LINK_FLAGS "-weak_framework 3DconnexionClient")
		message(STATUS "Found 3Dconnexion")
	        mark_as_advanced(CONNEXIONCLIENT_INCLUDE_DIR CONNEXIONCLIENT_LIBRARY)
	endif()
endif()

if (WIN32)
	find_path(CONNEXIONCLIENT_INCLUDE_DIRS I3dMouseParams.h PATH_SUFFIXES Inc HINTS ${CONNEXIONCLIENT_SEARCH_DIRS})

	include(FindPackageHandleStandardArgs)
	find_package_handle_standard_args(connexionClient DEFAULT_MSG CONNEXIONCLIENT_INCLUDE_DIRS)

	mark_as_advanced(CONNEXIONCLIENT_INCLUDE_DIRS CONNEXIONCLIENT_SEARCH_DIRS)
endif()
