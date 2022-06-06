# Declare dependencies
macro (setup_testcase_dependencies)
  # link in the shared libraries
  link_hifi_libraries(shared animation gpu hfm model-serializers graphics networking test-utils image)

  package_libraries_for_deployment()
endmacro ()

setup_hifi_testcase()
