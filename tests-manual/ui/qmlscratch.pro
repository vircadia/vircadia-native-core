TEMPLATE = app

QT += gui qml quick xml webengine widgets

CONFIG += c++11

SOURCES += src/main.cpp

# Additional import path used to resolve QML modules in Qt Creator's code model
QML_IMPORT_PATH = ../../interface/resources/qml

DISTFILES += \
    qml/*.qml
