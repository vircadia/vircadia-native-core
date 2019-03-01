//
//  Created by Bradley Austin Davis on 2015/06/19
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import Hifi 1.0 as Hifi
import QtQuick 2.5
import QtGraphicalEffects 1.0

import "./hifi/audio" as HifiAudio

import TabletScriptingInterface 1.0

Item {
    id: root;
    objectName: "AvatarInputsBar"
    property int modality: Qt.NonModal
    readonly property bool ignoreRadiusEnabled: AvatarInputs.ignoreRadiusEnabled
    width: audio.width;
    height: audio.height;
    x: 10;
    y: 5;
    readonly property bool shouldReposition: true;

    HifiAudio.MicBarApplication {
        id: audio;
        visible: AvatarInputs.showAudioTools;
        standalone: true;
        dragTarget: parent;
    }
    Rectangle {
        id: bubbleRect
        width: bubbleIcon.width + 10
        height: parent.height
        radius: 5;
        opacity: AvatarInputs.ignoreRadiusEnabled ? 0.7 : 0.3;

        color: "#00000000";
        border {
            width: mouseArea.containsMouse || mouseArea.containsPress ? 2 : 0;
            color: "#80FFFFFF";
        }
        anchors {
            left: root.right
            top: root.top
        }

        MouseArea {
            id: mouseArea;
            anchors.fill: parent

            hoverEnabled: true;
            scrollGestureEnabled: false;
            onClicked: {
                Tablet.playSound(TabletEnums.ButtonClick);
                Users.toggleIgnoreRadius();
            }
            drag.target: root;
            onContainsMouseChanged: {
                if (containsMouse) {
                    Tablet.playSound(TabletEnums.ButtonHover);
                }
            }
        }
        Image {
            id: bubbleIcon
            source: "../icons/tablet-icons/bubble-i.svg";
            sourceSize: Qt.size(28, 28);
            smooth: true;
            visible: false
            anchors.top: parent.top
            anchors.topMargin: (parent.height - bubbleIcon.height) / 2
            anchors.left: parent.left
            anchors.leftMargin: (parent.width - bubbleIcon.width) / 2
        }
        ColorOverlay {
            id: bubbleIconOverlay
            anchors.fill: bubbleIcon
            source: bubbleIcon
            color: "#FFFFFF";
        }
    }
}
