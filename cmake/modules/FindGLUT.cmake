# Try to find GLUT library and include path.
# Once done this will define
#
# GLUT_FOUND
# GLUT_INCLUDE_DIR
# GLUT_LIBRARIES
#
#  Created on 2/6/2014 by Stephen Birarda
# 
# Adapted from FindGLUT.cmake available in tlorach's OpenGLText Repository
# https://raw.github.com/tlorach/OpenGLText/master/cmake/FindGLUT.cmake

if (GLUT_INCLUDE_DIR AND GLUT_LIBRARIES)
  set(GLUT_FOUND TRUE)
else ()
  if (WIN32)
    set(WIN_GLUT_SEARCH_DIRS "${GLUT_ROOT_DIR}" "$ENV{GLUT_ROOT_DIR}" "$ENV{HIFI_LIB_DIR}/freeglut" "${OPENGL_INCLUDE_DIR}")
    find_path(GLUT_INCLUDE_DIR GL/glut.h PATH_SUFFIXES include HINTS ${WIN_GLUT_SEARCH_DIRS})
    
    if (CMAKE_CL_64)
      set(WIN_ARCH_DIR "lib/x64")
    else()
      set(WIN_ARCH_DIR "lib")
    endif()
    
    find_library(GLUT_glut_LIBRARY freeglut PATH_SUFFIXES ${WIN_ARCH_DIR} HINTS ${WIN_GLUT_SEARCH_DIRS})
  else ()
      find_path(GLUT_INCLUDE_DIR GL/glut.h
          "${GLUT_LOCATION}/include"
          "$ENV{GLUT_LOCATION}/include"
          /usr/include
          /usr/include/GL
          /usr/local/include
          /usr/openwin/share/include
          /usr/openwin/include
          /usr/X11R6/include
          /usr/include/X11
          /opt/graphics/OpenGL/include
          /opt/graphics/OpenGL/contrib/libglut
      )
      find_library(GLUT_glut_LIBRARY glut
          "${GLUT_LOCATION}/lib"
          "$ENV{GLUT_LOCATION}/lib"
          /usr/lib
          /usr/local/lib
          /usr/openwin/lib
          /usr/X11R6/lib
      )
      find_library(GLUT_Xi_LIBRARY Xi
          "${GLUT_LOCATION}/lib"
          "$ENV{GLUT_LOCATION}/lib"
          /usr/lib
          /usr/local/lib
          /usr/openwin/lib
          /usr/X11R6/lib
      )
      find_library(GLUT_Xmu_LIBRARY Xmu
          "${GLUT_LOCATION}/lib"
          "$ENV{GLUT_LOCATION}/lib"
          /usr/lib
          /usr/local/lib
          /usr/openwin/lib
          /usr/X11R6/lib
      )
  endif ()

  if (GLUT_INCLUDE_DIR AND GLUT_glut_LIBRARY)
    # Is -lXi and -lXmu required on all platforms that have it?
    # If not, we need some way to figure out what platform we are on.
    set(GLUT_LIBRARIES ${GLUT_glut_LIBRARY} ${GLUT_Xmu_LIBRARY} ${GLUT_Xi_LIBRARY})
  endif ()

  include(FindPackageHandleStandardArgs)
  find_package_handle_standard_args(GLUT DEFAULT_MSG GLUT_INCLUDE_DIR GLUT_LIBRARIES)
  
  if (GLUT_FOUND)
    if (NOT GLUT_FIND_QUIETLY)
      message(STATUS "Found GLUT: ${GLUT_LIBRARIES}")
    endif ()
  else ()
    if (GLUT_FIND_REQUIRED)
      message(FATAL_ERROR "Could not find GLUT")
    endif ()
  endif ()

  mark_as_advanced(GLUT_glut_LIBRARY GLUT_Xmu_LIBRARY GLUT_Xi_LIBRARY)

endif ()