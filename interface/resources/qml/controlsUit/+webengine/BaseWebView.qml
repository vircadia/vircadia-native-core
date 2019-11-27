//
//  WebView.qml
//
//  Created by Bradley Austin Davis on 12 Jan 2016
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import QtQuick 2.7
import QtWebEngine 1.5
import controlsUit 1.0 as ControlsUit

WebEngineView {
    id: root
    Component.onCompleted: {
        console.log("Connecting JS messaging to Hifi Logging")
        // Ensure the JS from the web-engine makes it to our logging
        root.javaScriptConsoleMessage.connect(function(level, message, lineNumber, sourceID) {
            console.log("Web Window JS message: " + sourceID + " " + lineNumber + " " +  message);
        });
    }

    onUrlChanged: {
        permissionPopupBackground.visible = false;
    }

    onLoadingChanged: {
        // Required to support clicking on "hifi://" links
        if (WebEngineView.LoadStartedStatus == loadRequest.status) {
            var url = loadRequest.url.toString();
            if (urlHandler.canHandleUrl(url)) {
                if (urlHandler.handleUrl(url)) {
                    root.stop();
                }
            }
        }
    }

    WebSpinner { }

    onFeaturePermissionRequested: {
        if (permissionPopupBackground.visible === true) {
            console.log("Browser engine requested a new permission, but user is already being presented with a different permission request. Aborting request for new permission...");
            return;
        }
        permissionPopupBackground.securityOrigin = securityOrigin;
        permissionPopupBackground.feature = feature;

        permissionPopupBackground.visible = true;
    }

    ControlsUit.PermissionPopupBackground {
        id: permissionPopupBackground
        z: 100
        onSendPermission: {
            root.grantFeaturePermission(securityOrigin, feature, shouldGivePermission);
        }
    }
}
