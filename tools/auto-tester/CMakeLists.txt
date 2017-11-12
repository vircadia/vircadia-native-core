set(TARGET_NAME auto-tester)

setup_hifi_project(Widgets)
link_hifi_libraries()

# grab the implementation and header files from src dirs
file(GLOB_RECURSE INTERFACE_SRCS "src/*.cpp" "src/*.h")
GroupSources("src")

find_package(
  Qt5 COMPONENTS
  Gui Multimedia Network OpenGL Qml Quick Script Svg
  ${PLATFORM_QT_COMPONENTS}
  WebChannel WebSockets
)

# grab the ui files in resources/ui
file (GLOB_RECURSE QT_UI_FILES ui/*.ui)
source_group("UI Files" FILES ${QT_UI_FILES})

# have qt5 wrap them and generate the appropriate header files
qt5_wrap_ui(QT_UI_HEADERS "${QT_UI_FILES}")

# add them to the interface source files
set(INTERFACE_SRCS ${INTERFACE_SRCS} "${QT_UI_HEADERS}" "${QT_RESOURCES}")
