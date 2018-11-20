TEMPLATE = app

QT += gui qml quick xml webview widgets

CONFIG += c++11

SOURCES += \
    cpp/main.cpp \
    cpp/PlayerWindow.cpp \
    cpp/RenderThread.cpp \
    cpp/resources.qrc

HEADERS += \
    cpp/PlayerWindow.h \
    cpp/RenderThread.h


# Additional import path used to resolve QML modules in Qt Creator's code model
QML_IMPORT_PATH = cpp

#DISTFILES += \
#    cpp/*.qml
