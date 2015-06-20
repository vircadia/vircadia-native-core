
# Declare dependencies
macro (setup_testcase_dependencies)
	#include_oglplus()

	# link in the shared libraries
	link_hifi_libraries(render-utils gpu shared)

	copy_dlls_beside_windows_executable()
endmacro ()

setup_hifi_testcase(Quick Gui OpenGL)