#  Find the OpenCV library
#
#  You must provide an OPENCV_ROOT_DIR which contains lib and include directories
#
#  Once done this will define
#
#  OPENCV_FOUND - system found OpenCV
#  OPENCV_INCLUDE_DIRS - the OpenCV include directory
#  OPENCV_LIBRARIES - Link this to use OpenCV
#
#  Created on 6/13/2013 by Andrzej Kapolka
#  Copyright (c) 2013 High Fidelity
#

if (OPENCV_LIBRARIES AND OPENCV_INCLUDE_DIRS)
  # in cache already
  set(OPENCV_FOUND TRUE)
else (OPENCV_LIBRARIES AND OPENCV_INCLUDE_DIRS)
  find_path(OPENCV_INCLUDE_DIRS opencv2/opencv.hpp ${OPENCV_ROOT_DIR}/include)

  foreach (MODULE core flann imgproc photo video features2d objdetect calib3d ml highgui contrib)
    if (APPLE)
        find_library(OPENCV_LIBRARY_${MODULE} libopencv_${MODULE}.a ${OPENCV_ROOT_DIR}/lib/MacOS/)
    elseif (UNIX)
        find_library(OPENCV_LIBRARY_${MODULE} libopencv_${MODULE}.a ${OPENCV_ROOT_DIR}/lib/UNIX/)
    elseif (WIN32)
        find_library(OPENCV_LIBRARY_${MODULE} opencv_${MODULE}.lib ${OPENCV_ROOT_DIR}/lib/Win32/)
    endif ()
    set(MODULE_LIBRARIES ${OPENCV_LIBRARY_${MODULE}} ${MODULE_LIBRARIES}) 
  endforeach (MODULE)
  set(OPENCV_LIBRARIES ${MODULE_LIBRARIES} CACHE STRING "OpenCV library paths")

  if (OPENCV_INCLUDE_DIRS AND OPENCV_LIBRARIES)
    set(OPENCV_FOUND TRUE)
  endif (OPENCV_INCLUDE_DIRS AND OPENCV_LIBRARIES)
 
  if (OPENCV_FOUND)
    if (NOT OPENCV_FIND_QUIETLY)
      message(STATUS "Found OpenCV: ${OPENCV_LIBRARIES}")
    endif (NOT OPENCV_FIND_QUIETLY)
  else (OPENCV_FOUND)
    if (OPENCV_FIND_REQUIRED)
      message(FATAL_ERROR "Could not find OpenCV")
    endif (OPENCV_FIND_REQUIRED)
  endif (OPENCV_FOUND)

  # show the OPENCV_INCLUDE_DIRS and OPENCV_LIBRARIES variables only in the advanced view
  mark_as_advanced(OPENCV_INCLUDE_DIRS OPENCV_LIBRARIES)

endif (OPENCV_LIBRARIES AND OPENCV_INCLUDE_DIRS)
