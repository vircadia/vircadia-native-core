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

AnimatedImage {
    source: "../../icons/loader-snake-64-w.gif"
    visible: parent.loading && /^(http.*|)$/i.test(parent.url.toString())
    z: 10000
    anchors {
        horizontalCenter: parent.horizontalCenter
        verticalCenter: parent.verticalCenter
    }
}
