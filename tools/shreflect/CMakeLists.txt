set(TARGET_NAME shreflect)

# don't use the setup_hifi_project macro as we don't want Qt or GLM dependencies
file(GLOB TARGET_SRCS src/*)
add_executable(${TARGET_NAME} ${TARGET_SRCS})
target_json()

if (WIN32)
 set_property(TARGET ${TARGET_NAME} APPEND_STRING PROPERTY LINK_FLAGS_DEBUG "/OPT:NOREF /OPT:NOICF")
endif()
