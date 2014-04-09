#
#  FindQxmpp.cmake
# 
#  Try to find the qxmpp library
#
#  You can provide a QXMPP_ROOT_DIR which contains lib and include directories
#
#  Once done this will define
#
#  QXMPP_FOUND - system found qxmpp
#  QXMPP_INCLUDE_DIRS - the qxmpp include directory
#  QXMPP_LIBRARIES - Link this to use qxmpp
#
#  Created on 3/10/2014 by Stephen Birarda
#  Copyright (c) 2014 High Fidelity
#
#  Distributed under the Apache License, Version 2.0.
#  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
# 

if (QXMPP_LIBRARIES AND QXMPP_INCLUDE_DIRS)
  # in cache already
  set(QXMPP_FOUND TRUE)
else ()
  
  set(QXMPP_SEARCH_DIRS "${QXMPP_ROOT_DIR}" "$ENV{HIFI_LIB_DIR}/qxmpp")
  
  find_path(QXMPP_INCLUDE_DIR QXmppClient.h PATH_SUFFIXES include/qxmpp HINTS ${QXMPP_SEARCH_DIRS})

  find_library(QXMPP_LIBRARY NAMES qxmpp qxmpp0 qxmpp_d PATH_SUFFIXES lib HINTS ${QXMPP_SEARCH_DIRS})
  
  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(QXMPP DEFAULT_MSG QXMPP_INCLUDE_DIR QXMPP_LIBRARY)
 
  if (QXMPP_FOUND)
    if (NOT QXMPP_FIND_QUIETLY)
      message(STATUS "Found qxmpp: ${QXMPP_LIBRARY}")
    endif ()
  else ()
    if (QXMPP_FIND_REQUIRED)
      message(FATAL_ERROR "Could not find qxmpp")
    endif ()
  endif ()
endif ()