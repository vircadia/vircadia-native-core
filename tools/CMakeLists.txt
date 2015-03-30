# add the tool directories
add_subdirectory(mtc)
set_target_properties(mtc PROPERTIES FOLDER "Tools")
add_subdirectory(scribe)
set_target_properties(scribe PROPERTIES FOLDER "Tools")


find_package(VHACD)
if(VHACD_FOUND)
add_subdirectory(vhacd)
# set_target_properties(vhacd PROPERTIES FOLDER "Tools")
endif()
