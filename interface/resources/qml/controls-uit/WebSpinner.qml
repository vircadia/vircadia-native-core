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
import QtWebEngine 1.5

AnimatedImage {
    property WebEngineView webview: parent
    source: "../../icons/loader-snake-64-w.gif"
    visible: webview.loading && /^(http.*|)$/i.test(webview.url.toString())
    playing: visible
    z: 10000
    anchors {
        horizontalCenter: parent.horizontalCenter
        verticalCenter: parent.verticalCenter
    }
}
