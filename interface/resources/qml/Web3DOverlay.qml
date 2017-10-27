//
//  Web3DOverlay.qml
//
//  Created by David Rowe on 16 Dec 2016.
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import QtQuick 2.5
import QtQuick.Controls 1.4

import "controls" as Controls

Controls.WebView {

    // This is for JS/QML communication, which is unused in a Web3DOverlay,
    // but not having this here results in spurious warnings about a
    // missing signal
    signal sendToScript(var message);

    function onWebEventReceived(event) {
        if (event.slice(0, 17) === "CLARA.IO DOWNLOAD") {
            ApplicationInterface.addAssetToWorldFromURL(event.slice(18));
        }
    }

    Component.onCompleted: {
        eventBridge.webEventReceived.connect(onWebEventReceived);
    }
}
