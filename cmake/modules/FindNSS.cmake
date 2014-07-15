#
#  FindNSS.cmake
# 
#  Try to find the Mozilla Network Security Services library
#
#  You can provide an NSS_ROOT_DIR which contains lib and include directories
#
#  Once done this will define
#
#  NSS_FOUND - system found NSS
#  NSS_INCLUDE_DIRS - the NSS include directory
#  NSS_LIBRARIES - Link this to use qxmpp
#
#  Created on 3/10/2014 by Stephen Birarda
#  Copyright 2014 High Fidelity, Inc.
#
#  Distributed under the Apache License, Version 2.0.
#  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
# 

if (NSS_LIBRARIES AND NSS_INCLUDE_DIRS)
  # in cache already
  set(NSS_FOUND TRUE)
else ()
  
  set(NSS_SEARCH_DIRS "${NSS_ROOT_DIR}" "$ENV{HIFI_LIB_DIR}/nss")
  
  find_path(NSS_INCLUDE_DIRS nss/nss.h PATH_SUFFIXES include HINTS ${NSS_SEARCH_DIRS})

  find_library(NSS_LIBRARY NAMES nss3 PATH_SUFFIXES lib HINTS ${NSS_SEARCH_DIRS})
  find_library(NSSUTIL_LIBRARY NAMES nssutil3 PATH_SUFFIXES lib HINTS ${NSS_SEARCH_DIRS})
  find_library(SMIME3_LIBRARY NAMES smime3 PATH_SUFFIXES lib HINTS ${NSS_SEARCH_DIRS})
  find_library(SSL3_LIBRARY NAMES ssl3 PATH_SUFFIXES lib HINTS ${NSS_SEARCH_DIRS})
  
  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(NSS DEFAULT_MSG NSS_INCLUDE_DIRS NSS_LIBRARY NSSUTIL_LIBRARY SMIME3_LIBRARY SSL3_LIBRARY)
  
  set(NSS_LIBRARIES "${NSS_LIBRARY} ${NSSUTIL_LIBRARY} ${SMIME3_LIBRARY} ${SSL3_LIBRARY}")
endif ()