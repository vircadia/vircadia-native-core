# Declare dependencies
macro (setup_testcase_dependencies)
  # link in the shared libraries
  link_hifi_libraries(shared animation gpu fbx model networking)

  package_libraries_for_deployment()
endmacro ()

setup_hifi_testcase()
