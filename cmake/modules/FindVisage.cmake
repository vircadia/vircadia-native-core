# 
#  FindVisage.cmake
# 
#  Try to find the Visage controller library
#
#  You must provide a VISAGE_ROOT_DIR which contains lib and include directories
#
#  Once done this will define
#
#  VISAGE_FOUND - system found Visage
#  VISAGE_INCLUDE_DIRS - the Visage include directory
#  VISAGE_LIBRARIES - Link this to use Visage
#
#  Created on 2/11/2014 by Andrzej Kapolka
#  Copyright 2014 High Fidelity, Inc.
#
#  Distributed under the Apache License, Version 2.0.
#  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
# 

include("${MACRO_DIR}/HifiLibrarySearchHints.cmake")
hifi_library_search_hints("visage")

find_path(VISAGE_BASE_INCLUDE_DIR VisageTracker2.h PATH_SUFFIXES include HINTS ${VISAGE_SEARCH_DIRS})

if (APPLE)
  find_path(VISAGE_XML_INCLUDE_DIR libxml/xmlreader.h HINTS /usr/include/libxml2 ${VISAGE_SEARCH_DIRS})
  find_path(VISAGE_OPENCV_INCLUDE_DIR cv.h PATH_SUFFIXES dependencies/OpenCV_MacOSX/include HINTS ${VISAGE_SEARCH_DIRS})
  find_path(VISAGE_OPENCV2_INCLUDE_DIR opencv.hpp PATH_SUFFIXES dependencies/OpenCV_MacOSX/include/opencv2 HINTS ${VISAGE_SEARCH_DIRS})
  
  find_library(VISAGE_CORE_LIBRARY NAME vscore PATH_SUFFIXES lib HINTS ${VISAGE_SEARCH_DIRS})
  find_library(VISAGE_VISION_LIBRARY NAME vsvision PATH_SUFFIXES lib HINTS ${VISAGE_SEARCH_DIRS})
  find_library(VISAGE_OPENCV_LIBRARY NAME OpenCV PATH_SUFFIXES dependencies/OpenCV_MacOSX/lib HINTS ${VISAGE_SEARCH_DIRS})
  
  find_library(QuartzCore QuartzCore)
elseif (WIN32)
  find_path(VISAGE_XML_INCLUDE_DIR libxml/xmlreader.h PATH_SUFFIXES dependencies/libxml2/include HINTS ${VISAGE_SEARCH_DIRS})
  find_path(VISAGE_OPENCV_INCLUDE_DIR opencv/cv.h PATH_SUFFIXES dependencies/OpenCV/include HINTS ${VISAGE_SEARCH_DIRS})
  find_path(VISAGE_OPENCV2_INCLUDE_DIR cv.h PATH_SUFFIXES dependencies/OpenCV/include/opencv HINTS ${VISAGE_SEARCH_DIRS})
  
  find_library(VISAGE_CORE_LIBRARY NAME vscore PATH_SUFFIXES lib HINTS ${VISAGE_SEARCH_DIRS})
  find_library(VISAGE_VISION_LIBRARY NAME vsvision PATH_SUFFIXES lib HINTS ${VISAGE_SEARCH_DIRS})
  find_library(VISAGE_OPENCV_LIBRARY NAME opencv_core243 PATH_SUFFIXES dependencies/OpenCV/lib HINTS ${VISAGE_SEARCH_DIRS})
endif ()

include(FindPackageHandleStandardArgs)

set(VISAGE_ARGS_LIST "VISAGE_BASE_INCLUDE_DIR VISAGE_XML_INCLUDE_DIR 
  VISAGE_OPENCV_INCLUDE_DIR VISAGE_OPENCV2_INCLUDE_DIR
  VISAGE_CORE_LIBRARY VISAGE_VISION_LIBRARY VISAGE_OPENCV_LIBRARY")
  
if (APPLE)
  list(APPEND VISAGE_ARGS_LIST QuartzCore)
endif ()

find_package_handle_standard_args(VISAGE DEFAULT_MSG 
  VISAGE_ARGS_LIST
)

set(VISAGE_INCLUDE_DIRS "${VISAGE_XML_INCLUDE_DIR}" "${VISAGE_OPENCV_INCLUDE_DIR}" "${VISAGE_OPENCV2_INCLUDE_DIR}" "${VISAGE_BASE_INCLUDE_DIR}")
set(VISAGE_LIBRARIES "${VISAGE_CORE_LIBRARY}" "${VISAGE_VISION_LIBRARY}" "${VISAGE_OPENCV_LIBRARY}")

if (APPLE)
  list(APPEND VISAGE_LIBRARIES ${QuartzCore})
endif ()

mark_as_advanced(
  VISAGE_INCLUDE_DIRS VISAGE_LIBRARIES
  VISAGE_BASE_INCLUDE_DIR VISAGE_XML_INCLUDE_DIR VISAGE_OPENCV_INCLUDE_DIR VISAGE_OPENCV2_INCLUDE_DIR
  VISAGE_CORE_LIBRARY VISAGE_VISION_LIBRARY VISAGE_OPENCV_LIBRARY VISAGE_SEARCH_DIRS
)
