//
//  AssetDialog.qml
//
//  Created by David Rowe on 18 Apr 2017
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import QtQuick 2.8

import stylesUit 1.0
import "../../windows"

ScrollingWindow {
    id: root
    resizable: false
    implicitWidth: 480
    implicitHeight: 706
    objectName: "aboutDialog"
    destroyOnCloseButton: true
    destroyOnHidden: true
    visible: true
    minSize: Qt.vector2d(480, 706)
    title: "About"

    TabletAboutDialog {
        width: pane.width
        height: pane.height
    }
}
