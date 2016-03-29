import QtQuick 2.3
import QtQuick.Controls 1.2
import QtWebEngine 1.1
import QtWebChannel 1.0

import "windows" as Windows
import "controls" as Controls
import "styles"

Windows.Window {
    id: root
    HifiConstants { id: hifi }
    title: "WebWindow"
    resizable: true
    visible: false
    // Don't destroy on close... otherwise the JS/C++ will have a dangling pointer
    destroyOnCloseButton: false
    property alias source: webview.url
    property alias eventBridge: eventBridgeWrapper.eventBridge;

    QtObject {
        id: eventBridgeWrapper
        WebChannel.id: "eventBridgeWrapper"
        property var eventBridge;
    }

    // This is for JS/QML communication, which is unused in a WebWindow,
    // but not having this here results in spurious warnings about a 
    // missing signal
    signal sendToScript(var message);

    Controls.WebView {
        id: webview
        url: "about:blank"
        anchors.fill: parent
        focus: true
        webChannel.registeredObjects: [eventBridgeWrapper]
    }
} // dialog
