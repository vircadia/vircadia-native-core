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
#  Copyright (c) 2014 High Fidelity
#

if (VISAGE_LIBRARIES AND VISAGE_INCLUDE_DIRS)
  # in cache already
  set(VISAGE_FOUND TRUE)
else ()
  find_path(VISAGE_INCLUDE_DIR VisageTracker2.h ${VISAGE_ROOT_DIR}/include)
  
  if (APPLE)
    find_path(VISAGE_XML_INCLUDE_DIR libxml/xmlreader.h /usr/include/libxml2)
    find_path(VISAGE_OPENCV_INCLUDE_DIR cv.h ${VISAGE_ROOT_DIR}/dependencies/OpenCV_MacOSX/include)
    find_path(VISAGE_OPENCV2_INCLUDE_DIR opencv.hpp ${VISAGE_ROOT_DIR}/dependencies/OpenCV_MacOSX/include/opencv2)
    if (VISAGE_INCLUDE_DIR AND VISAGE_XML_INCLUDE_DIR AND VISAGE_OPENCV_INCLUDE_DIR AND VISAGE_OPENCV2_INCLUDE_DIR)
      set(VISAGE_INCLUDE_DIRS
        "${VISAGE_INCLUDE_DIR};${VISAGE_XML_INCLUDE_DIR};${VISAGE_OPENCV_INCLUDE_DIR};${VISAGE_OPENCV2_INCLUDE_DIR}"
        CACHE INTERNAL "Visage include dirs")
    endif (VISAGE_INCLUDE_DIR AND VISAGE_XML_INCLUDE_DIR AND VISAGE_OPENCV_INCLUDE_DIR AND VISAGE_OPENCV2_INCLUDE_DIR)
    
    find_library(VISAGE_CORE_LIBRARY libvscore.a ${VISAGE_ROOT_DIR}/lib)
    find_library(VISAGE_VISION_LIBRARY libvsvision.a ${VISAGE_ROOT_DIR}/lib)
    find_library(VISAGE_OPENCV_LIBRARY libOpenCV.a ${VISAGE_ROOT_DIR}/dependencies/OpenCV_MacOSX/lib)
    if (VISAGE_CORE_LIBRARY AND VISAGE_VISION_LIBRARY AND VISAGE_OPENCV_LIBRARY)
      set(VISAGE_LIBRARIES "${VISAGE_CORE_LIBRARY};${VISAGE_VISION_LIBRARY};${VISAGE_OPENCV_LIBRARY}"
        CACHE INTERNAL "Visage libraries")
    endif (VISAGE_CORE_LIBRARY AND VISAGE_VISION_LIBRARY AND VISAGE_OPENCV_LIBRARY)
    
  elseif (WIN32)
    find_path(VISAGE_XML_INCLUDE_DIR libxml/xmlreader.h ${VISAGE_ROOT_DIR}/dependencies/libxml2/include)
    find_path(VISAGE_OPENCV_INCLUDE_DIR opencv/cv.h ${VISAGE_ROOT_DIR}/dependencies/OpenCV/include)
    find_path(VISAGE_OPENCV2_INCLUDE_DIR cv.h ${VISAGE_ROOT_DIR}/dependencies/OpenCV/include/opencv)
    if (VISAGE_INCLUDE_DIR AND VISAGE_XML_INCLUDE_DIR AND VISAGE_OPENCV_INCLUDE_DIR AND VISAGE_OPENCV2_INCLUDE_DIR)
      set(VISAGE_INCLUDE_DIRS
        "${VISAGE_INCLUDE_DIR};${VISAGE_XML_INCLUDE_DIR};${VISAGE_OPENCV_INCLUDE_DIR};${VISAGE_OPENCV2_INCLUDE_DIR}"
        CACHE INTERNAL "Visage include dirs")
    endif (VISAGE_INCLUDE_DIR AND VISAGE_XML_INCLUDE_DIR AND VISAGE_OPENCV_INCLUDE_DIR AND VISAGE_OPENCV2_INCLUDE_DIR)
    
    find_library(VISAGE_CORE_LIBRARY vscore.lib ${VISAGE_ROOT_DIR}/lib)
    find_library(VISAGE_VISION_LIBRARY vsvision.lib ${VISAGE_ROOT_DIR}/lib)
    find_library(VISAGE_OPENCV_LIBRARY opencv_core243.lib ${VISAGE_ROOT_DIR}/dependencies/OpenCV/lib)
    if (VISAGE_CORE_LIBRARY AND VISAGE_VISION_LIBRARY AND VISAGE_OPENCV_LIBRARY)
      set(VISAGE_LIBRARIES "${VISAGE_CORE_LIBRARY};${VISAGE_VISION_LIBRARY};${VISAGE_OPENCV_LIBRARY}"
        CACHE INTERNAL "Visage libraries")
    endif (VISAGE_CORE_LIBRARY AND VISAGE_VISION_LIBRARY AND VISAGE_OPENCV_LIBRARY)
    
  endif ()

  if (VISAGE_INCLUDE_DIRS AND VISAGE_LIBRARIES)
     set(VISAGE_FOUND TRUE)
  endif (VISAGE_INCLUDE_DIRS AND VISAGE_LIBRARIES)
 
  if (VISAGE_FOUND)
    if (NOT VISAGE_FIND_QUIETLY)
      message(STATUS "Found Visage: ${VISAGE_LIBRARIES}")
    endif (NOT VISAGE_FIND_QUIETLY)
  else (VISAGE_FOUND)
    if (VISAGE_FIND_REQUIRED)
      message(FATAL_ERROR "Could not find Visage")
    endif (VISAGE_FIND_REQUIRED)
  endif (VISAGE_FOUND)

  # show the VISAGE_INCLUDE_DIRS and VISAGE_LIBRARIES variables only in the advanced view
  mark_as_advanced(VISAGE_INCLUDE_DIRS VISAGE_LIBRARIES)

endif ()
