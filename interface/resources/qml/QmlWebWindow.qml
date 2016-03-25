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
    property alias webChannel: webview.webChannel
    // A unique identifier to let the HTML JS find the event bridge 
    // object (our C++ wrapper)
    property string uid;

    // This is for JS/QML communication, which is unused in a WebWindow,
    // but not having this here results in spurious warnings about a 
    // missing signal
    signal sendToScript(var message);

    Controls.WebView {
        id: webview
        url: "about:blank"
        anchors.fill: parent
        focus: true
        onUrlChanged: webview.runJavaScript("EventBridgeUid = \"" + uid + "\";");
        Component.onCompleted: webview.runJavaScript("EventBridgeUid = \"" + uid + "\";");
    }
} // dialog
