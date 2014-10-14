set(TARGET_NAME networking)

# use setup_hifi_library macro to setup our project and link appropriate Qt modules
setup_hifi_library(Network)

link_hifi_libraries(shared)

if (WIN32)
  # we need ws2_32.lib on windows, but it's static so we don't bubble it up
  target_link_libraries(${TARGET_NAME} ws2_32.lib)
endif ()

# find OpenSSL
find_package(OpenSSL REQUIRED)

include_directories(SYSTEM "${OPENSSL_INCLUDE_DIR}")

# append OpenSSL to our list of libraries to link
list(APPEND ${TARGET_NAME}_LIBRARIES_TO_LINK "${OPENSSL_LIBRARIES}")

# call macro to link our dependencies and bubble them up via a property on our target
link_shared_dependencies()