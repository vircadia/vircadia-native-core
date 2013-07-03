#  Find the static SKETCHUP library
#
#  You must provide an SKETCHUP_ROOT_DIR which contains framework
#
#  Once done this will define
#
#  SKETCHUP_FOUND - system found SketchUp API
#  SKETCHUP_FRAMEWORK - Link this to use PortAudio
#
#  Created on 6/27/2013 by Stojce Slavkovski
#  Copyright (c) 2013 High Fidelity
#

if (SKETCHUP_FRAMEWORK)
  # in cache already
  set(SKETCHUP_FOUND TRUE)
else (SKETCHUP_FRAMEWORK)
	
  find_library(SKETCHUP_FRAMEWORK slapi ${SKETCHUP_ROOT_DIR})

  if (SKETCHUP_FRAMEWORK)
    set(SKETCHUP_FOUND TRUE)
  endif (SKETCHUP_FRAMEWORK)

  if (SKETCHUP_FOUND)
    if (NOT SKETCHUP_FIND_QUIETLY)
      message(STATUS "Found SketchUp: ${SKETCHUP_FRAMEWORK}")
    endif (NOT SKETCHUP_FIND_QUIETLY)
  else (SKETCHUP_FOUND)
    if (SKETCHUP_FIND_REQUIRED)
      message(FATAL_ERROR "Could not find SketchUp")
    endif (SKETCHUP_FIND_REQUIRED)
  endif (SKETCHUP_FOUND)

  # show the SKETCHUP_FRAMEWORK variables only in the advanced view
  mark_as_advanced(SKETCHUP_FRAMEWORK)

endif (SKETCHUP_FRAMEWORK)