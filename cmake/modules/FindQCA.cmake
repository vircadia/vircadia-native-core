#  Try to find the QCA library
#
#  You can provide a QCA_ROOT_DIR which contains lib and include directories
#
#  Once done this will define
#
#  QCA_FOUND - system found qca
#  QCA_INCLUDE_DIRS - the qca include directory
#  QCA_LIBRARIES - Link this to use qca
#
#  Created on 3/28/2014 by Stephen Birarda
#  Copyright (c) 2014 High Fidelity
#

if (QCA_LIBRARIES AND QCA_INCLUDE_DIRS)
  # in cache already
  set(QCA_FOUND TRUE)
else ()
  
  set(QCA_SEARCH_DIRS "${QCA_ROOT_DIR}" "$ENV{HIFI_LIB_DIR}/qca")
  
  find_path(QCA_INCLUDE_DIR qca.h PATH_SUFFIXES include/QtCrypto HINTS ${QCA_SEARCH_DIRS})

  find_library(QCA_LIBRARY NAMES qca PATH_SUFFIXES lib HINTS ${QCA_SEARCH_DIRS})
  
  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(QCA DEFAULT_MSG QCA_INCLUDE_DIR QCA_LIBRARY)
 
  if (QCA_FOUND)
    if (NOT QCA_FIND_QUIETLY)
      message(STATUS "Found qca: ${QCA_LIBRARY}")
    endif ()
  else ()
    if (QCA_FIND_REQUIRED)
      message(FATAL_ERROR "Could not find qca")
    endif ()
  endif ()
endif ()