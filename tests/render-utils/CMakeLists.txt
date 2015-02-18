set(TARGET_NAME render-utils-tests)
 
setup_hifi_project(Quick Gui OpenGL)

#include_oglplus()

# link in the shared libraries
link_hifi_libraries(render-utils gpu shared)

if (WIN32)
  # we're using static GLEW, so define GLEW_STATIC
  add_definitions(-DGLEW_STATIC)
endif ()

copy_dlls_beside_windows_executable()