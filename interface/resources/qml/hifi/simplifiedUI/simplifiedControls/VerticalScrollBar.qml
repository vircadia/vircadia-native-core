//
//  VerticalScrollBar.qml
//
//  Created by Zach Fox on 2019-06-17
//  Copyright 2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import QtQuick 2.10
import QtQuick.Controls 2.3
import "../simplifiedConstants" as SimplifiedConstants

ScrollBar {
    SimplifiedConstants.SimplifiedConstants {
        id: simplifiedUI
    }

    orientation: Qt.Vertical
    policy: ScrollBar.AlwaysOn
    anchors.top: parent.top
    anchors.topMargin: 4
    anchors.right: parent.right
    anchors.rightMargin: 4
    anchors.bottom: parent.bottom
    anchors.bottomMargin: 4
    width: simplifiedUI.sizes.controls.scrollBar.backgroundWidth
    visible: parent.contentHeight > parent.parent.height
    position: parent.contentY / parent.contentHeight
    size: parent.parent.height / parent.contentHeight
    minimumSize: 0.1
    background: Rectangle {
        color: simplifiedUI.colors.controls.scrollBar.background
        anchors.fill: parent
    }
    contentItem: Rectangle {
        width: simplifiedUI.sizes.controls.scrollBar.contentItemWidth
        color: simplifiedUI.colors.controls.scrollBar.contentItem
        anchors {
            horizontalCenter: parent.horizontalCenter
            topMargin: 1
            bottomMargin: 1
        }
    }
    onPositionChanged: {
        if (pressed) {
            parent.contentY = position * parent.contentHeight;
        }
    }
}