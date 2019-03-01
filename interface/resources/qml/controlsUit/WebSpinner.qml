//
//  WebSpinner.qml
//
//  Created by David Rowe on 23 May 2017
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import QtQuick 2.5

Image {
    Item {
        id: webView
        property bool loading: false
        property string url: ""
    }

    source: "qrc:////images//unsupportedImage.png"
    visible: webview.loading && /^(http.*|)$/i.test(webview.url.toString())
    playing: visible
    z: 10000
    anchors {
        horizontalCenter: parent.horizontalCenter
        verticalCenter: parent.verticalCenter
    }
}
