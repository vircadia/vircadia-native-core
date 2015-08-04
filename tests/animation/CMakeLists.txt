# Declare dependencies
macro (setup_testcase_dependencies)
  # link in the shared libraries
  link_hifi_libraries(shared animation gpu fbx model)

  copy_dlls_beside_windows_executable()
endmacro ()

setup_hifi_testcase()
