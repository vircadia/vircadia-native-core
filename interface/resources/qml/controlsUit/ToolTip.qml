//
//  ToolTip.qml
//
//  Created by Clement on  9/12/17
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import QtQuick 2.5

Item {
    property string toolTip
    property bool showToolTip: false

    Rectangle {
        id: toolTipRectangle
        anchors.right: parent.right

        width: toolTipText.width + 4
        height: toolTipText.height + 4
        opacity: (toolTip != "" && showToolTip) ? 1 : 0
        color: "#ffffaa"
        border.color: "#0a0a0a"
        Text {
            id: toolTipText
            text: toolTip
            color: "black"
            anchors.centerIn: parent
        }
        Behavior on opacity {
            PropertyAnimation {
                easing.type: Easing.InOutQuad
                duration: 250
            }
        }
    }
    MouseArea {
        id: mouseArea
        anchors.fill: parent
        onEntered: showTimer.start()
        onExited: { showToolTip = false; showTimer.stop(); }
        hoverEnabled: true
    }
    Timer {
        id: showTimer
        interval: 250
        onTriggered: { showToolTip = true; }
    }
}