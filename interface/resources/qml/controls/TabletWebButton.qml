//
//  TabletWebButton.qml
//
//  Created by Dante Ruiz on 2017/4/13
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import Hifi 1.0
import QtQuick 2.4
import "../styles-uit"

Rectangle {
    property alias text: label.text
    property alias pixelSize: label.font.pixelSize;
    property bool selected: false
    property bool hovered: false
    property int spacing: 2
    property var action: function () {}
    property string enabledColor: hifi.colors.blueHighlight
    property string disabledColor: hifi.colors.blueHighlight
    width: label.width + 64
    height: 32
    color: hifi.colors.white
    enabled: false

    HifiConstants { id: hifi }

    RalewaySemiBold {
        id: label;
        color: enabled ? enabledColor : disabledColor
        font.pixelSize: 15;
        anchors {
            horizontalCenter: parent.horizontalCenter;
            verticalCenter: parent.verticalCenter;
        }
    }
        
    Rectangle {
        id: indicator
        width: parent.width
        height: selected ? 3 : 1
        anchors {
            left: parent.left
            right: parent.right
            bottom: parent.bottom
        }
        color: hifi.colors.blueHighlight
        visible: parent.selected || hovered
    }
}
