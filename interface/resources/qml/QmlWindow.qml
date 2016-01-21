
import QtQuick 2.3
import QtQuick.Controls 1.2
import QtWebChannel 1.0
import QtWebSockets 1.0
import "qrc:///qtwebchannel/qwebchannel.js" as WebChannel

import "windows" as Windows
import "controls"
import "styles"

Windows.Window {
    id: root
    HifiConstants { id: hifi }
    title: "QmlWindow"
    resizable: true
    visible: false
    focus: true
    property var channel;
    // Don't destroy on close... otherwise the JS/C++ will have a dangling pointer
    destroyOnCloseButton: false
    property alias source: pageLoader.source 
    
    Loader { 
        id: pageLoader
        objectName: "Loader"
        focus: true
        property var dialog: root
    }
} // dialog
