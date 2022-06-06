
# Declare dependencies
macro (setup_testcase_dependencies)
  link_hifi_libraries(shared workload shaders)
  package_libraries_for_deployment()
endmacro ()

setup_hifi_testcase()
