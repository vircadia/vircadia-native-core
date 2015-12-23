
import QtQuick 2.3
import QtQuick.Controls 1.2
import QtWebChannel 1.0
import QtWebSockets 1.0

import "controls"
import "styles"

VrDialog {
    id: root
    HifiConstants { id: hifi }
    title: "QmlWindow"
    resizable: true
	enabled: false
    // Don't destroy on close... otherwise the JS/C++ will have a dangling pointer
    destroyOnCloseButton: false
    contentImplicitWidth: clientArea.implicitWidth
    contentImplicitHeight: clientArea.implicitHeight
    property url source: "" 

    onEnabledChanged: {
        if (enabled) {
            clientArea.forceActiveFocus()
        }
    }

    Item {
        id: clientArea
        implicitHeight: 600
        implicitWidth: 800
        x: root.clientX
        y: root.clientY
        width: root.clientWidth
        height: root.clientHeight

        Loader { 
            id: pageLoader
            objectName: "Loader"
            source: root.source
            anchors.fill: parent
        }
    } // item
} // dialog
