
import QtQuick 2.3
import QtQuick.Controls 1.2
import QtWebEngine 1.1
import QtWebChannel 1.0
import QtWebSockets 1.0

import "qwebchannel.js" as WebChannel
import "controls"
import "styles"

VrDialog {
    id: root
    HifiConstants { id: hifi }
    title: "WebWindow"
    resizable: true
    contentImplicitWidth: clientArea.implicitWidth
    contentImplicitHeight: clientArea.implicitHeight
    backgroundColor: "#7f000000"
    property url source: "about:blank"

    Component.onCompleted: {
        enabled = true
        console.log("Web Window Created " + root);
        webview.javaScriptConsoleMessage.connect(function(level, message, lineNumber, sourceID) {
            console.log("Web Window JS message: " + sourceID + " " + lineNumber + " " +  message);
        });
    }

    Item {
        id: clientArea
        implicitHeight: 600
        implicitWidth: 800
        x: root.clientX
        y: root.clientY
        width: root.clientWidth
        height: root.clientHeight
        
        WebEngineView {
            id: webview
            url: root.source
            anchors.fill: parent
        }
    } // item
} // dialog
