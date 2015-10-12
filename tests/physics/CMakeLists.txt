
# Declare dependencies
macro (SETUP_TESTCASE_DEPENDENCIES)
  target_bullet()
  link_hifi_libraries(shared physics)
  copy_dlls_beside_windows_executable()
endmacro ()

setup_hifi_testcase(Script)
