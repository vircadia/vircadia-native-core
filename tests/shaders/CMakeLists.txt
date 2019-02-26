
# Declare dependencies
macro (setup_testcase_dependencies)
  # link in the shared libraries
  link_hifi_libraries(shared test-utils gpu shaders gl ${PLATFORM_GL_BACKEND})
  #target_spirv()

  find_package(spirv_cross_core REQUIRED)
  find_package(spirv_cross_reflect REQUIRED)
  find_package(spirv_cross_glsl REQUIRED)
  target_include_directories(${TARGET_NAME} PRIVATE $<TARGET_PROPERTY:spirv-cross-core,INTERFACE_INCLUDE_DIRECTORIES>)
  target_include_directories(${TARGET_NAME} PRIVATE $ENV{VULKAN_SDK}/Include)
  
 
  package_libraries_for_deployment()
endmacro ()

setup_hifi_testcase(Gui)

