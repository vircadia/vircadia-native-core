set(TARGET_NAME auto-tester)

setup_hifi_project(Widgets)

# grab the ui files in ui
file (GLOB_RECURSE QT_UI_FILES src/ui/*.ui)
source_group("UI Files" FILES ${QT_UI_FILES})

# have qt5 wrap them and generate the appropriate header files
qt5_wrap_ui(QT_UI_HEADERS "${QT_UI_FILES}")

# add them to the interface source files
set(INTERFACE_SRCS ${INTERFACE_SRCS} "${QT_UI_HEADERS}")
