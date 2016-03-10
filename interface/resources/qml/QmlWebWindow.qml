import QtQuick 2.3
import QtQuick.Controls 1.2
import QtWebEngine 1.1

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

    Controls.WebView {
        id: webview
        url: "about:blank"
        anchors.fill: parent
        focus: true
    }
} // dialog
