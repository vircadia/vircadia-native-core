#
#  Find3DConnexionClient.cmake
# 
#  Once done this will define

#  3DCONNEXIONCLIENT_FOUND - system found 3DConnexion
#  3DCONNEXIONCLIENT_INCLUDE_DIRS - the 3DConnexion include directory
#  3DCONNEXIONCLIENT_LIBRARY - Link this to use 3DConnexion
# 
#  Created on 10/06/2015 by Marcel Verhagen
#  Copyright 2015 High Fidelity, Inc.
# 
#  Distributed under the Apache License, Version 2.0.
#  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
# 

include("${MACRO_DIR}/HifiLibrarySearchHints.cmake")
hifi_library_search_hints("3dconnexionclient")

if (APPLE)
  find_library(3DCONNEXIONCLIENT 3DconnexionClient)
  if(EXISTS ${3DCONNEXIONCLIENT})
    find_path(3DCONNEXIONCLIENT_INCLUDE_DIR2 ConnexionClient.h PATH_SUFFIXES include HINTS ${3DCONNEXIONCLIENT_SEARCH_DIRS})
    include_directories(${3DCONNEXIONCLIENT_INCLUDE_DIR2})
        
    get_filename_component( 3DCONNEXIONCLIENT_FRAMEWORK_DIR ${3DCONNEXIONCLIENT} PATH )
    set_target_properties(${TARGET_NAME} PROPERTIES LINK_FLAGS "-weak_framework 3DconnexionClient")
    set_target_properties(${TARGET_NAME} PROPERTIES FRAMEWORK_SEARCH_PATHS 3DCONNEXIONCLIENT_FRAMEWORK_DIR)

    include(FindPackageHandleStandardArgs)
    find_package_handle_standard_args(3DCONNEXIONCLIENT DEFAULT_MSG 3DCONNEXIONCLIENT_INCLUDE_DIR2)
    mark_as_advanced(3DCONNEXIONCLIENT_INCLUDE_DIR2) 
    message(STATUS "Found 3DConnexion")
  else ()
    message(STATUS "Could NOT find 3DConnexionClient")
  endif()
elseif (WIN32)
  find_path(3DCONNEXIONCLIENT_INCLUDE_DIRS I3dMouseParams.h PATH_SUFFIXES include HINTS ${3DCONNEXIONCLIENT_SEARCH_DIRS})

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(3DCONNEXIONCLIENT DEFAULT_MSG 3DCONNEXIONCLIENT_INCLUDE_DIRS)

  mark_as_advanced(3DCONNEXIONCLIENT_INCLUDE_DIRS 3DCONNEXIONCLIENT_SEARCH_DIRS)
endif()
