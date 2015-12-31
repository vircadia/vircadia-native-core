
import QtQuick 2.3
import QtQuick.Controls 1.2
import QtWebChannel 1.0
import QtWebSockets 1.0
import "qrc:///qtwebchannel/qwebchannel.js" as WebChannel

import "controls"
import "styles"

VrDialog {
    id: root
    objectName: "topLevelWindow"
    HifiConstants { id: hifi }
    title: "QmlWindow"
    resizable: true
    enabled: false
    visible: false
    focus: true
    property var channel;
    
    // Don't destroy on close... otherwise the JS/C++ will have a dangling pointer
    destroyOnCloseButton: false
    contentImplicitWidth: clientArea.implicitWidth
    contentImplicitHeight: clientArea.implicitHeight
    property alias source: pageLoader.source 

    Item {
        id: clientArea
        implicitHeight: 600
        implicitWidth: 800
        x: root.clientX
        y: root.clientY
        width: root.clientWidth
        height: root.clientHeight
        focus: true
        clip: true

        Loader { 
            id: pageLoader
            objectName: "Loader"
            anchors.fill: parent
            focus: true
            property var dialog: root
            
            onLoaded: {
                forceActiveFocus()
            }
            
            Keys.onPressed: {
                console.log("QmlWindow pageLoader keypress")
            }
        }
    } // item
} // dialog
