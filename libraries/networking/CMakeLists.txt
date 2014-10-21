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

if (APPLE AND ${OPENSSL_INCLUDE_DIR} STREQUAL "/usr/include")
  # this is a user on OS X using system OpenSSL, which is going to throw warnings since they're deprecating for their common crypto
  message(WARNING "The found version of OpenSSL is the OS X system version. This will produce deprecation warnings."
   "\nWe recommend you install a newer version (at least 1.0.1h) in a different directory and set OPENSSL_ROOT_DIR in your env so Cmake can find it.")
endif ()

include_directories(SYSTEM "${OPENSSL_INCLUDE_DIR}")

# append OpenSSL to our list of libraries to link
list(APPEND ${TARGET_NAME}_LIBRARIES_TO_LINK "${OPENSSL_LIBRARIES}")

# call macro to link our dependencies and bubble them up via a property on our target
link_shared_dependencies()