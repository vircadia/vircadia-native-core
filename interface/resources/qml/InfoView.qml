//
//  InfoView.qml
//
//  Created by Bradley Austin Davis on 27 Apr 2015
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import QtQuick 2.5
import Hifi 1.0 as Hifi

import "controls-uit"
import "windows" as Windows

Windows.ScrollingWindow {
    id: root
    width: 800
    height: 800
    resizable: true
    
    Hifi.InfoView {
        id: infoView
        width: pane.contentWidth
        implicitHeight: pane.scrollHeight

        WebView {
            id: webview
            objectName: "WebView"
            anchors.fill: parent
            url: infoView.url
        }
    }

    Component.onCompleted: {
        centerWindow(root);
    }

    onVisibleChanged: {
        if (visible) {
            centerWindow(root);
        }
    }

    function centerWindow() {
        desktop.centerOnVisible(root);
    }

}
