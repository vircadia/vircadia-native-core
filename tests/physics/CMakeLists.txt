
# Declare dependencies
macro (SETUP_TESTCASE_DEPENDENCIES)
  target_bullet()
  link_hifi_libraries(shared physics gpu model)
  package_libraries_for_deployment()
endmacro ()

setup_hifi_testcase(Script)
