# add the tool directories
add_subdirectory(scribe)
set_target_properties(scribe PROPERTIES FOLDER "Tools")

add_subdirectory(udt-test)
set_target_properties(udt-test PROPERTIES FOLDER "Tools")

add_subdirectory(vhacd-util)
set_target_properties(vhacd-util PROPERTIES FOLDER "Tools")

add_subdirectory(ice-client)
set_target_properties(ice-client PROPERTIES FOLDER "Tools")

add_subdirectory(ac-client)
set_target_properties(ac-client PROPERTIES FOLDER "Tools")

add_subdirectory(skeleton-dump)
set_target_properties(skeleton-dump PROPERTIES FOLDER "Tools")

