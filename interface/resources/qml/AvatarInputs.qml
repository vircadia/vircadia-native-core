//
//  Created by Bradley Austin Davis on 2015/06/19
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import Hifi 1.0 as Hifi
import QtQuick 2.4
import QtQuick.Controls 1.3
import QtGraphicalEffects 1.0
import Qt.labs.settings 1.0

Hifi.AvatarInputs {
    id: root
    objectName: "AvatarInputs"
    width: mirrorWidth
    height: controls.height + mirror.height 
    x: 10; y: 5

    readonly property int mirrorHeight: 215
    readonly property int mirrorWidth: 265
    readonly property int iconSize: 24
    readonly property int iconPadding: 5

    readonly property bool shouldReposition: true

    Settings {
        category: "Overlay.AvatarInputs"
        property alias x: root.x
        property alias y: root.y
    }
    
    MouseArea {
        id: hover
        hoverEnabled: true
        drag.target: parent
        anchors.fill: parent
    }

    Item {
        id: mirror
        width: root.mirrorWidth
        height: root.mirrorVisible ? root.mirrorHeight : 0
        visible: root.mirrorVisible
        anchors.left: parent.left
        clip: true

        Image {
            id: closeMirror
            visible: hover.containsMouse
            width: root.iconSize
            height: root.iconSize
            anchors.top: parent.top
            anchors.topMargin: root.iconPadding
            anchors.left: parent.left
            anchors.leftMargin: root.iconPadding
            source: "../images/close.svg"
            MouseArea {
                anchors.fill: parent
                onClicked: {
                    root.closeMirror();
                }
            }
        }

        Image {
            id: zoomIn
            visible: hover.containsMouse
            width: root.iconSize
            height: root.iconSize
            anchors.bottom: parent.bottom
            anchors.bottomMargin: root.iconPadding
            anchors.left: parent.left
            anchors.leftMargin: root.iconPadding
            source: root.mirrorZoomed ? "../images/minus.svg" : "../images/plus.svg"
            MouseArea {
                anchors.fill: parent
                onClicked: {
                    root.toggleZoom();
                }
            }
        }
    }

    Item {
        id: controls
        width: root.mirrorWidth
        height: 44
        visible: root.showAudioTools
        anchors.top: mirror.bottom

        Rectangle {
            anchors.fill: parent
            color: root.mirrorVisible ? (root.audioClipping ? "red" : "#696969") : "#00000000"

            Item {
                id: audioMeter
                anchors.verticalCenter: parent.verticalCenter
                anchors.left: parent.left
                anchors.leftMargin: root.iconPadding
                anchors.right: parent.right
                anchors.rightMargin: root.iconPadding
                height: 8
                Rectangle {
                    id: blueRect
                    color: "blue"
                    anchors.top: parent.top
                    anchors.bottom: parent.bottom
                    anchors.left: parent.left
                    width: parent.width / 4
                }
                Rectangle {
                    id: greenRect
                    color: "green"
                    anchors.top: parent.top
                    anchors.bottom: parent.bottom
                    anchors.left: blueRect.right
                    anchors.right: redRect.left
                }
                Rectangle {
                    id: redRect
                    color: "red"
                    anchors.top: parent.top
                    anchors.bottom: parent.bottom
                    anchors.right: parent.right
                    width: parent.width / 5
                }
                Rectangle {
                    z: 100
                    anchors.top: parent.top
                    anchors.bottom: parent.bottom
                    anchors.right: parent.right
                    width: (1.0 - root.audioLevel) * parent.width
                    color: "black"
                }
           }
        }
    }
}

