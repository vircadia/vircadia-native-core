//
//  WebEntityView.qml
//
//  Created by Kunal Gosar on 16 March 2017
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import QtQuick 2.5
import QtWebEngine 1.5
import Hifi 1.0

/*
TestItem {
    Rectangle {
        anchors.fill: parent
        anchors.margins: 10
        color: "blue"
        property string url: ""
        ColorAnimation on color {
            loops: Animation.Infinite; from: "blue"; to: "yellow"; duration: 1000 
        }
    }
}
*/

WebEngineView {
    id: webViewCore
    objectName: "webEngineView"
    width: parent !== null ? parent.width : undefined
    height: parent !== null ? parent.height : undefined

    onFeaturePermissionRequested: {
        grantFeaturePermission(securityOrigin, feature, true);
    }

    //disable popup
    onContextMenuRequested: {
        request.accepted = true;
    }

    onNewViewRequested: {
        newViewRequestedCallback(request)
    }
}
