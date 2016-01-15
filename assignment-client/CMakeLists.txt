set(TARGET_NAME assignment-client)

setup_hifi_project(Core Gui Network Script Quick Widgets WebSockets)

# link in the shared libraries
link_hifi_libraries(
  audio avatars octree environment gpu model fbx entities
  networking animation recording shared script-engine embedded-webserver
  controllers physics
)

if (WIN32)
  # we need SSL_EAY_LIBRARY_* to be set before we call package_libraries_for_deployment()
  # so we have to call find_package(OpenSSL) here even though this target doesn't specifically need it
  find_package(OpenSSL REQUIRED)
  set(MANUAL_SSLEAY_COPY TRUE)
  package_libraries_for_deployment()
endif()

install_beside_console()
