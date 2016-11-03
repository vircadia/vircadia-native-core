set(TARGET_NAME shared)

# TODO: there isn't really a good reason to have Script linked here - let's get what is requiring it out (RegisteredMetaTypes.cpp)
setup_hifi_library(Gui Network Script Widgets)

if (WIN32)
    target_link_libraries(${TARGET_NAME} Wbemuuid.lib)
endif()

target_zlib()
target_nsight()