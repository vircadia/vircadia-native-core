set(TARGET_NAME webview)
setup_hifi_project(Gui WebView)
link_hifi_libraries(shared ktx gl qml)
target_opengl()

set(EXTRA_DEPLOY_OPTIONS "--qmldir  \"${PROJECT_SOURCE_DIR}/src\"")
package_libraries_for_deployment()