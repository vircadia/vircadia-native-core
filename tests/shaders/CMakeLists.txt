# This doesn't link properly, and it seems it's built against some older library
# not being in use anymore. At least some of the stuff in the test doesn't seem
# to match anything in the build tree. If built, it fails with:

# [ 91%] Linking CXX executable shaders-ShaderTests
# ...
# ShaderTests.cpp:354: undefined reference to `glslang::TShader::TShader(EShLanguage)'
# (other errors snipped)
#
# So I've disabled the entire thing, but left the code around as a reference in case it can be fixed.

# Declare dependencies
#macro (setup_testcase_dependencies)
#  # link in the shared libraries
#  link_hifi_libraries(shared test-utils gpu shaders gl ${PLATFORM_GL_BACKEND})
#  #target_spirv()
#
#  find_package(spirv_cross_core REQUIRED)
#  find_package(spirv_cross_reflect REQUIRED)
#  find_package(spirv_cross_glsl REQUIRED)
#  target_include_directories(${TARGET_NAME} PRIVATE $<TARGET_PROPERTY:spirv-cross-core,INTERFACE_INCLUDE_DIRECTORIES>)
#  target_include_directories(${TARGET_NAME} PRIVATE $ENV{VULKAN_SDK}/Include)
#
#
#  package_libraries_for_deployment()
#endmacro ()
#
#setup_hifi_testcase(Gui)

