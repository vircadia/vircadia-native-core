set(TARGET_NAME oven)

setup_hifi_project(Widgets Gui Concurrent)

link_hifi_libraries(model-baking shared image gpu ktx)

if (WIN32)
  package_libraries_for_deployment()
endif ()
