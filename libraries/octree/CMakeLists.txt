set(TARGET_NAME octree)
include_directories(system /usr/local/Cellar/qt5/5.9.1/include)
setup_hifi_library()
link_hifi_libraries(shared networking)
