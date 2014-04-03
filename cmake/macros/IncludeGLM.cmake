macro(INCLUDE_GLM TARGET ROOT_DIR)

	find_package(GLM REQUIRED)
	include_directories("${GLM_INCLUDE_DIRS}")

  if (APPLE OR UNIX)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -isystem ${GLM_INCLUDE_DIRS}")
  endif ()

endmacro(INCLUDE_GLM _target _root_dir)