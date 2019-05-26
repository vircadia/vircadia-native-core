//
//  PropItem.qml
//
//  Created by Sam Gateau on 3/2/2019
//  Copyright 2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0.html
//

import QtQuick 2.7

import controlsUit 1.0 as HifiControls

PropItem {
    Global { id: global }
    id: root

    // Scalar Prop
    property bool integral: false
    property var numDigits: 2

    PropLabel {
        id: valueLabel

        anchors.left: root.splitter.right
        anchors.right: root.right
        anchors.verticalCenter: root.verticalCenter
        horizontalAlignment: global.valueTextAlign
        height: global.slimHeight
        
        text:  root.valueVarGetter();

        background: Rectangle {
            color: global.color
            border.color: global.colorBorderLight
            border.width: global.valueBorderWidth
            radius: global.valueBorderRadius
        }
    }
}
