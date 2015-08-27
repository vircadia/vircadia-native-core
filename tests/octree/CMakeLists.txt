
# Declare dependencies
macro (setup_testcase_dependencies)
  # link in the shared libraries
  link_hifi_libraries(shared octree gpu model fbx networking environment entities avatars audio animation script-engine physics)

  copy_dlls_beside_windows_executable()
endmacro ()

setup_hifi_testcase(Script Network)
