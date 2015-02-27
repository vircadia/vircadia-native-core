# add the tool directories
add_subdirectory(bitstream2json)
add_subdirectory(json2bitstream)
add_subdirectory(mtc)
add_subdirectory(scribe)

find_package(VHACD)
if(VHACD_FOUND)
add_subdirectory(vhacd)
endif()
