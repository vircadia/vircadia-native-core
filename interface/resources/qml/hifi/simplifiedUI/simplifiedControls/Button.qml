//
//  Button.qml
//
//  Created by Zach Fox on 2019-05-08
//  Copyright 2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import QtQuick 2.10
import QtQuick.Controls 2.3 as Original
import stylesUit 1.0 as HifiStylesUit
import "../simplifiedConstants" as SimplifiedConstants
import TabletScriptingInterface 1.0

Original.Button {
    id: root

    SimplifiedConstants.SimplifiedConstants {
        id: simplifiedUI
    }

    hoverEnabled: true
    width: simplifiedUI.sizes.controls.button.width
    height: simplifiedUI.sizes.controls.button.height

    onHoveredChanged: {
        if (hovered && enabled) {
            Tablet.playSound(TabletEnums.ButtonHover);
        }
    }

    onFocusChanged: {
        if (focus && enabled) {
            Tablet.playSound(TabletEnums.ButtonHover);
        }
    }

    onClicked: {
        if (enabled) {
            Tablet.playSound(TabletEnums.ButtonClick);
        }
    }

    background: Rectangle {
        implicitWidth: root.width
        implicitHeight: root.height
        color: {
            if (root.enabled) {
                if (root.hovered) {
                    simplifiedUI.colors.controls.button.background.enabled
                } else if (root.down) {
                    simplifiedUI.colors.controls.button.background.active
                } else {
                    simplifiedUI.colors.controls.button.background.enabled
                }
            } else {
                simplifiedUI.colors.controls.button.background.disabled
            }
        }

        border.width: simplifiedUI.sizes.controls.button.borderWidth
        border.color: root.enabled ? simplifiedUI.colors.controls.button.border.enabled : simplifiedUI.colors.controls.button.border.disabled

        Item {
            clip: true
            visible: root.enabled
            anchors.centerIn: parent
            width: parent.width - parent.border.width * 2
            height: parent.height - parent.border.width * 2
            
            Rectangle {
                z: -1
                clip: true
                width: root.down ? parent.width * 1.5 : (root.hovered ? parent.width * 9 / 10 : 0)
                height: parent.height
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.horizontalCenterOffset: -14
                color: simplifiedUI.colors.controls.button.background.active
                Behavior on width {
                    enabled: true
                    SmoothedAnimation { velocity: 400 }
                }
                transform: Matrix4x4 {
                    property real a: Math.PI / 4
                    matrix: Qt.matrix4x4(1, Math.tan(a), 0, 0,
                                        0,            1, 0, 0,
                                        0,            0, 1, 0,
                                        0,            0, 0, 1)
                }
            }
        }
    }

    contentItem:  HifiStylesUit.GraphikMedium {
        id: buttonText
        topPadding: -2 // Necessary for proper alignment using Graphik Medium
        wrapMode: Text.Wrap
        color: enabled ? simplifiedUI.colors.controls.button.text.enabled : simplifiedUI.colors.controls.button.text.disabled
        size: simplifiedUI.sizes.controls.button.textSize
        verticalAlignment: Text.AlignVCenter
        horizontalAlignment: Text.AlignHCenter
        text: root.text
    }
}
