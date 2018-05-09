# Declare dependencies
macro (setup_testcase_dependencies)
  # link in the shared libraries
  link_hifi_libraries(shared ktx gpu gl ${PLATFORM_GL_BACKEND})
  package_libraries_for_deployment()
  target_opengl()
endmacro ()

setup_hifi_testcase()
