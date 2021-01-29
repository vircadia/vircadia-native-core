//
//  Web3DSurface.qml
//
//  Created by David Rowe on 16 Dec 2016.
//  Copyright 2016 High Fidelity, Inc.
//  Copyright 2020 Vircadia contributors.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import QtQuick 2.5

import "controls" as Controls

Item {
    id: root
    anchors.fill: parent
    property string url: ""
    property string scriptUrl: null
    property bool useBackground: true
    property string userAgent: ""

    onUrlChanged: {
        load(root.url, root.scriptUrl, root.useBackground, root.userAgent);
    }

    onScriptUrlChanged: {
        if (root.item) {
            root.item.scriptUrl = root.scriptUrl;
        } else {
            load(root.url, root.scriptUrl, root.useBackground, root.userAgent);
        }
    }

    onUseBackgroundChanged: {
        if (root.item) {
            root.item.useBackground = root.useBackground;
        } else {
            load(root.url, root.scriptUrl, root.useBackground, root.userAgent);
        }
    }
    
    onUserAgentChanged: {
        if (root.item) {
            root.item.userAgent = root.userAgent;
        } else {
            load(root.url, root.scriptUrl, root.useBackground, root.userAgent);
        }
    }

    property var item: null

    function load(url, scriptUrl, useBackground, userAgent) {
        // Ensure we reset any existing item to "about:blank" to ensure web audio stops: DEV-2375
        if (root.item != null) {
            root.item.url = "about:blank"
            root.item.destroy()
            root.item = null
        }
        QmlSurface.load("./controls/WebView.qml", root, function(newItem) {
            root.item = newItem
            root.item.url = url
            root.item.scriptUrl = scriptUrl
            root.item.useBackground = useBackground
            root.item.userAgent = userAgent
        })
    }

    Component.onCompleted: {
        load(root.url, root.scriptUrl, root.useBackground, root.userAgent);
    }

    signal sendToScript(var message);
}
