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
hifi_library_search_hints("connexionclient")

if (APPLE)
  find_library(3DCONNEXIONCLIENT_LIBRARIES NAMES 3DConnexionClient HINTS 3DCONNEXIONCLIENT_SEARCH_DIRS)
  if(EXISTS ${3DConnexionClient})
    set(3DCONNEXIONCLIENT_FOUND true)
    set(3DCONNEXIONCLIENT_INCLUDE_DIRS ${3DConnexionClient})
    set(3DCONNEXIONCLIENT_LIBRARY ${3DConnexionClient})
    message(STATUS "Found 3DConnexion at " ${3DConnexionClient})
    mark_as_advanced(3DCONNEXIONCLIENT_INCLUDE_DIR 3DCONNEXIONCLIENT_LIBRARY)
  else ()
    message(STATUS "Could NOT find 3DConnexionClient")
  endif()
elseif (WIN32)
  find_path(3DCONNEXIONCLIENT_INCLUDE_DIRS I3dMouseParams.h PATH_SUFFIXES include HINTS ${3DCONNEXIONCLIENT_SEARCH_DIRS})

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(3DConnexionClient DEFAULT_MSG 3DCONNEXIONCLIENT_INCLUDE_DIRS)

  mark_as_advanced(3DCONNEXIONCLIENT_INCLUDE_DIRS 3DCONNEXIONCLIENT_SEARCH_DIRS)
endif()