# Declare dependencies
macro (SETUP_TESTCASE_DEPENDENCIES)
  # link in the shared libraries
  link_hifi_libraries(shared audio networking)

  copy_dlls_beside_windows_executable()
endmacro ()

setup_memory_debugger()

setup_hifi_testcase()
