# Declare dependencies
macro (SETUP_TESTCASE_DEPENDENCIES)
  # link in the shared libraries
  link_hifi_libraries(shared ktx gpu image)

  package_libraries_for_deployment()
endmacro ()

setup_hifi_testcase()
