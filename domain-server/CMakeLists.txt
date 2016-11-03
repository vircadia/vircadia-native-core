set(TARGET_NAME domain-server)

if (UPPER_CMAKE_BUILD_TYPE MATCHES DEBUG AND NOT WIN32)
  set(_SHOULD_SYMLINK_RESOURCES TRUE)
else ()
  set(_SHOULD_SYMLINK_RESOURCES FALSE)
endif ()

# setup the project and link required Qt modules
setup_hifi_project(Network)

# Fix up the rpath so macdeployqt works
if (APPLE)
  set_target_properties(${TARGET_NAME} PROPERTIES INSTALL_RPATH "@executable_path/../Frameworks")
endif ()

# TODO: find a solution that will handle web file changes in resources on windows without a re-build.
# Currently the resources are only copied on post-build. If one is changed but the domain-server is not, they will
# not be re-copied. This is worked-around on OS X/UNIX by using a symlink.
symlink_or_copy_directory_beside_target(${_SHOULD_SYMLINK_RESOURCES} "${CMAKE_CURRENT_SOURCE_DIR}/resources" "resources")

# link the shared hifi libraries
link_hifi_libraries(embedded-webserver networking shared)

# find OpenSSL
find_package(OpenSSL REQUIRED)

if (APPLE AND ${OPENSSL_INCLUDE_DIR} STREQUAL "/usr/include")
  # this is a user on OS X using system OpenSSL, which is going to throw warnings since they're deprecating for their common crypto
  message(WARNING "The found version of OpenSSL is the OS X system version. This will produce deprecation warnings."
    "\nWe recommend you install a newer version (at least 1.0.1h) in a different directory and set OPENSSL_ROOT_DIR in your env so Cmake can find it.")
endif ()

include_directories(SYSTEM "${OPENSSL_INCLUDE_DIR}")

# append OpenSSL to our list of libraries to link
target_link_libraries(${TARGET_NAME} ${OPENSSL_LIBRARIES})

# libcrypto uses dlopen in libdl
if (UNIX)
  target_link_libraries(${TARGET_NAME} ${CMAKE_DL_LIBS})
endif ()

if (WIN32)
  package_libraries_for_deployment()
endif ()

install_beside_console()
