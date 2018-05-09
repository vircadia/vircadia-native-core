# Declare dependencies
macro (setup_testcase_dependencies)
  # link in the shared libraries
  link_hifi_libraries(shared test-utils ktx gpu gl ${PLATFORM_GL_BACKEND})
  package_libraries_for_deployment()
  target_opengl()
  target_zlib()
  find_package(QuaZip REQUIRED)
  target_include_directories(${TARGET_NAME} SYSTEM PUBLIC ${QUAZIP_INCLUDE_DIRS})
  target_link_libraries(${TARGET_NAME} ${QUAZIP_LIBRARIES})
  if (WIN32)
    add_paths_to_fixup_libs(${QUAZIP_DLL_PATH})
    add_dependency_external_projects(wasapi)
  endif ()
endmacro ()

setup_hifi_testcase()
