TEMPLATE = app

QT += gui qml quick xml webengine widgets

CONFIG += c++11

SOURCES += src/main.cpp \
    ../../libraries/ui/src/FileDialogHelper.cpp

HEADERS += \
    ../../libraries/ui/src/FileDialogHelper.h

# Additional import path used to resolve QML modules in Qt Creator's code model
QML_IMPORT_PATH = ../../interface/resources/qml


DISTFILES += \
    qml/*.qml \
    ../../interface/resources/qml/*.qml \
    ../../interface/resources/qml/controls/*.qml \
    ../../interface/resources/qml/controls-uit/*.qml \
    ../../interface/resources/qml/dialogs/*.qml \
    ../../interface/resources/qml/dialogs/fileDialog/*.qml \
    ../../interface/resources/qml/dialogs/preferences/*.qml \
    ../../interface/resources/qml/dialogs/messageDialog/*.qml \
    ../../interface/resources/qml/desktop/*.qml \
    ../../interface/resources/qml/menus/*.qml \
    ../../interface/resources/qml/styles/*.qml \
    ../../interface/resources/qml/styles-uit/*.qml \
    ../../interface/resources/qml/windows/*.qml \
    ../../interface/resources/qml/hifi/*.qml \
    ../../interface/resources/qml/hifi/toolbars/*.qml \
    ../../interface/resources/qml/hifi/dialogs/*.qml \
    ../../interface/resources/qml/hifi/dialogs/preferences/*.qml \
    ../../interface/resources/qml/hifi/overlays/*.qml
