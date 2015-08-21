
# Declare dependencies
macro (setup_testcase_dependencies)
  # link in the shared libraries
  link_hifi_libraries(shared networking)

  copy_dlls_beside_windows_executable()
endmacro ()

setup_memory_debugger()

setup_hifi_testcase()
