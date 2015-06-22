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

Hifi.AvatarInputs {
    id: root
    objectName: "AvatarInputs"
    anchors.fill: parent

//    width: 800
//    height: 600
//    color: "black"
    readonly property int iconPadding: 5
    readonly property int mirrorHeight: 215
    readonly property int mirrorWidth: 265
    readonly property int mirrorTopPad: iconPadding
    readonly property int mirrorLeftPad: 10
    readonly property int iconSize: 24

    Item {
        id: mirror
        width: root.mirrorWidth
        height: root.mirrorVisible ? root.mirrorHeight : 0
        visible: root.mirrorVisible
        anchors.left: parent.left
        anchors.leftMargin: root.mirrorLeftPad
        anchors.top: parent.top
        anchors.topMargin: root.mirrorTopPad
        clip: true

        MouseArea {
             id: hover
             anchors.fill: parent
             hoverEnabled: true
             propagateComposedEvents: true
         }

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
            visible:  hover.containsMouse
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
        width: root.mirrorWidth
        height: 44

        x: root.mirrorLeftPad
        y: root.mirrorVisible ? root.mirrorTopPad + root.mirrorHeight : 5



        Rectangle {
            anchors.fill: parent
            color: root.mirrorVisible ? (root.audioClipping ? "red" : "#696969") : "#00000000"

            Image {
                id: faceMute
                width: root.iconSize
                height: root.iconSize
                visible: root.cameraEnabled
                anchors.left: parent.left
                anchors.leftMargin: root.iconPadding
                anchors.verticalCenter: parent.verticalCenter
                source: root.cameraMuted ? "../images/face-mute.svg" : "../images/face.svg"
                MouseArea {
                    anchors.fill: parent
                    onClicked: {
                        root.toggleCameraMute()
                    }
                    onDoubleClicked: {
                        root.resetSensors();
                    }
                }
            }

            Image {
                id: micMute
                width: root.iconSize
                height: root.iconSize
                anchors.left: root.cameraEnabled ? faceMute.right : parent.left
                anchors.leftMargin: root.iconPadding
                anchors.verticalCenter: parent.verticalCenter
                source: root.audioMuted ? "../images/mic-mute.svg" : "../images/mic.svg"
                MouseArea {
                    anchors.fill: parent
                    onClicked: {
                        root.toggleAudioMute()
                    }
                }
            }

            Item {
                id: audioMeter
                anchors.verticalCenter: parent.verticalCenter
                anchors.left: micMute.right
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

