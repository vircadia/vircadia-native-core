set(TARGET_NAME assignment-client)

setup_hifi_project(Core Gui Network Script Widgets)

add_dependency_external_projects(glm)
find_package(GLM REQUIRED)
target_include_directories(${TARGET_NAME} PRIVATE ${GLM_INCLUDE_DIRS})

# link in the shared libraries
link_hifi_libraries( 
  audio avatars octree environment gpu model fbx entities 
  networking animation shared script-engine embedded-webserver
  physics
)

if (UNIX)
  target_link_libraries(${TARGET_NAME} ${CMAKE_DL_LIBS})
endif (UNIX)

include_application_version()
copy_dlls_beside_windows_executable()

if (DEFINED ENV{HIFI_MEMORY_DEBUGGING})
  if (UNIX)
    MESSAGE("-- assignment-client memory debugging is enabled.")
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address -fno-omit-frame-pointer")
    SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=address")
  endif (UNIX)
endif ()
