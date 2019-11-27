# Declare dependencies
macro (setup_testcase_dependencies)
  # link in the shared libraries
  link_hifi_libraries(shared test-utils ktx gpu gl ${PLATFORM_GL_BACKEND})
  package_libraries_for_deployment()
  target_opengl()
  target_zlib()
  target_quazip()
  if (WIN32)
    add_dependency_external_projects(wasapi)
  endif ()
endmacro ()

setup_hifi_testcase()
