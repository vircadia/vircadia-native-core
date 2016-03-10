set(TARGET_NAME udt-test)
setup_hifi_project()

set_target_properties(${TARGET_NAME} PROPERTIES EXCLUDE_FROM_ALL TRUE EXCLUDE_FROM_DEFAULT_BUILD TRUE)

link_hifi_libraries(networking shared)
package_libraries_for_deployment()
