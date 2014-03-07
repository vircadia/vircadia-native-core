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

if (WIN32)
    find_path( GLUT_INCLUDE_DIR GL/glut.h
        "${GLUT_ROOT_DIR}/include"
        "$ENV{GLUT_ROOT_DIR}/include"
        "$ENV{HIFI_LIB_DIR}/freeglut/include"
        "${OPENGL_INCLUDE_DIR}"
        DOC "The directory where GL/glut.h resides")
    if(ARCH STREQUAL "x86")
      find_library( GLUT_glut_LIBRARY
        NAMES freeglut
        PATHS
        "${GLUT_ROOT_DIR}/lib"
        "$ENV{GLUT_ROOT_DIR}/lib"
        "$ENV{HIFI_LIB_DIR}/freeglut/lib"
        DOC "The GLUT library")
    else()
      find_library( GLUT_glut_LIBRARY
        NAMES freeglut
        PATHS
        "${GLUT_ROOT_DIR}/lib/x64"
        "$ENV{GLUT_ROOT_DIR}/lib/x64"
        "$ENV{HIFI_LIB_DIR}/freeglut/lib/x64"
        DOC "The GLUT library")
    endif()
else ()
    find_path( GLUT_INCLUDE_DIR GL/glut.h
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
    find_library( GLUT_glut_LIBRARY glut
        "${GLUT_LOCATION}/lib"
        "$ENV{GLUT_LOCATION}/lib"
        /usr/lib
        /usr/local/lib
        /usr/openwin/lib
        /usr/X11R6/lib
    )
    find_library( GLUT_Xi_LIBRARY Xi
        "${GLUT_LOCATION}/lib"
        "$ENV{GLUT_LOCATION}/lib"
        /usr/lib
        /usr/local/lib
        /usr/openwin/lib
        /usr/X11R6/lib
    )
    find_library( GLUT_Xmu_LIBRARY Xmu
        "${GLUT_LOCATION}/lib"
        "$ENV{GLUT_LOCATION}/lib"
        /usr/lib
        /usr/local/lib
        /usr/openwin/lib
        /usr/X11R6/lib
    )
endif (WIN32)

set(GLUT_FOUND "NO")

if(GLUT_INCLUDE_DIR)
  if(GLUT_glut_LIBRARY)
    # Is -lXi and -lXmu required on all platforms that have it?
    # If not, we need some way to figure out what platform we are on.
    set(GLUT_LIBRARIES
      ${GLUT_glut_LIBRARY}
      ${GLUT_Xmu_LIBRARY}
      ${GLUT_Xi_LIBRARY}
    )
    set( GLUT_FOUND "YES")

    set (GLUT_LIBRARY ${GLUT_LIBRARIES})
    set (GLUT_INCLUDE_PATH ${GLUT_INCLUDE_DIR})

  endif(GLUT_glut_LIBRARY)
endif(GLUT_INCLUDE_DIR)

include(FindPackageHandleStandardArgs)

find_package_handle_standard_args(GLUT DEFAULT_MSG
    GLUT_INCLUDE_DIR
    GLUT_LIBRARIES
)

mark_as_advanced(
  GLUT_INCLUDE_DIR
  GLUT_glut_LIBRARY
  GLUT_Xmu_LIBRARY
  GLUT_Xi_LIBRARY
)
