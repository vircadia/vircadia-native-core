cmake_minimum_required(VERSION 3.5)

find_package(Catch2 REQUIRED)
add_executable(vircadia-client-tests version.cpp client.cpp messages.cpp avatars.cpp)
target_link_libraries(vircadia-client-tests PRIVATE vircadia-client Catch2::Catch2WithMain)

include(CTest)
include(Catch)
catch_discover_tests(vircadia-client-tests)
