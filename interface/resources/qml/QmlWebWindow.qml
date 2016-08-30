//
//  QmlWebWindow.qml
//
//  Created by Bradley Austin Davis on 17 Dec 2015
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import QtQuick 2.5
import QtQuick.Controls 1.4
import QtWebEngine 1.1
import QtWebChannel 1.0

import "windows" as Windows
import "controls-uit" as Controls
import "styles-uit"

Windows.ScrollingWindow {
    id: root
    HifiConstants { id: hifi }
    title: "WebWindow"
    resizable: true
    shown: false
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

    signal moved(vector2d position);
    signal resized(size size);

    function notifyMoved() {
        moved(Qt.vector2d(x, y));
    }

    function notifyResized() {
        resized(Qt.size(width, height));
    }

    onXChanged: notifyMoved();
    onYChanged: notifyMoved();

    onWidthChanged: notifyResized();
    onHeightChanged: notifyResized();

    Item {
        width: pane.contentWidth
        implicitHeight: pane.scrollHeight

        Controls.WebView {
            id: webview
            url: "about:blank"
            anchors.fill: parent
            focus: true
            webChannel.registeredObjects: [eventBridgeWrapper]
        }
    }
}
