//
//  InputDeviceButton.qml
//
//  Created by Zach Fox on 2019-05-02
//  Based off of MicBarApplication.qml by Zach Pomerantz and Wayne Chen
//  Copyright 2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import QtQuick 2.10
import QtGraphicalEffects 1.0
import stylesUit 1.0
import TabletScriptingInterface 1.0
import "../simplifiedConstants" as SimplifiedConstants

Rectangle {
    id: micBar

    SimplifiedConstants.SimplifiedConstants {
        id: simplifiedUI
    }

    readonly property var level: AudioScriptingInterface.inputLevel
    readonly property var clipping: AudioScriptingInterface.clipping
    property var muted: AudioScriptingInterface.muted
    property var pushToTalk: AudioScriptingInterface.pushToTalk
    property var pushingToTalk: AudioScriptingInterface.pushingToTalk
    readonly property var userSpeakingLevel: 0.4
    property bool gated: false

    readonly property string unmutedIcon: "images/mic-unmute-i.svg"
    readonly property string mutedIcon: "images/mic-mute-i.svg"
    readonly property string pushToTalkIcon: "images/mic-ptt-i.svg"
    readonly property string clippingIcon: "images/mic-clip-i.svg"
    readonly property string gatedIcon: "images/mic-gate-i.svg"
    
    Connections {
        target: AudioScriptingInterface

        onNoiseGateOpened: {
            gated = false;
        }

        onNoiseGateClosed: {
            gated = false;
        }
    }

    height: 30
    width: 34

    opacity: 0.7

    onLevelChanged: {
        var rectOpacity = (muted && (level >= userSpeakingLevel)) ? 1.0 : 0.7;
        if (pushToTalk && !pushingToTalk) {
            rectOpacity = (mouseArea.containsMouse) ? 1.0 : 0.7;
        } else if (mouseArea.containsMouse && rectOpacity != 1.0) {
            rectOpacity = 1.0;
        }
        micBar.opacity = rectOpacity;
    }

    color: "#00000000"

    MouseArea {
        id: mouseArea

        anchors {
            left: icon.left
            right: bar.right
            top: icon.top
            bottom: icon.bottom
        }

        hoverEnabled: true
        scrollGestureEnabled: false
        onClicked: {
            if (pushToTalk) {
                return;
            }
            AudioScriptingInterface.muted = !muted;
            Tablet.playSound(TabletEnums.ButtonClick);
            muted = Qt.binding(function() { return AudioScriptingInterface.muted; }); // restore binding
        }
        onContainsMouseChanged: {
            if (containsMouse) {
                Tablet.playSound(TabletEnums.ButtonHover);
            }
        }
    }

    QtObject {
        id: colors

        readonly property string unmutedColor: simplifiedUI.colors.controls.inputVolumeButton.text.noisy
        readonly property string gatedColor: "#00BDFF"
        readonly property string mutedColor: simplifiedUI.colors.controls.inputVolumeButton.text.muted
        readonly property string gutter: "#575757"
        readonly property string greenStart: "#39A38F"
        readonly property string greenEnd: "#1FC6A6"
        readonly property string yellow: "#C0C000"
        readonly property string fill: "#55000000"
        readonly property string icon: (muted || clipping) ? mutedColor : gated ? gatedColor : unmutedColor
    }

    Item {
        id: icon
        anchors.verticalCenter: parent.verticalCenter
        anchors.left: parent.left
        width: parent.width - bar.width - bar.anchors.leftMargin
        height: parent.height

        Item {
            anchors.fill: parent
            opacity: mouseArea.containsMouse ? 1.0 : 0.7
            Image {
                id: image
                source: (pushToTalk) ? pushToTalkIcon : muted ? mutedIcon :
                    clipping ? clippingIcon : gated ? gatedIcon : unmutedIcon
                anchors.fill: parent
                fillMode: Image.PreserveAspectFit
            }

            ColorOverlay {
                id: imageOverlay
                anchors { fill: image }
                source: image
                color: pushToTalk ? (pushingToTalk ? colors.unmutedColor : colors.mutedColor) : colors.icon
            }
        }
    }

    Item {
        id: bar

        anchors {
            left: icon.right
            leftMargin: 0
            verticalCenter: icon.verticalCenter
        }

        width: 4
        height: parent.height

        Rectangle { // base
            id: baseBar
            radius: 4
            anchors { fill: parent }
            color: colors.gutter
        }

        Rectangle { // mask
            id: mask
            height: micBar.muted ? parent.height : parent.height * level
            color: micBar.muted ? colors.mutedColor : "white"
            width: parent.width
            radius: 5
            anchors {
                bottom: parent.bottom
                bottomMargin: 0
                left: parent.left
                leftMargin: 0
            }
        }

        LinearGradient {
            anchors { fill: mask }
            visible: mask.visible && !micBar.muted
            source: mask
            start: Qt.point(0, 0)
            end: Qt.point(0, bar.height)
            rotation: 180
            gradient: Gradient {
                GradientStop {
                    position: 0.0
                    color: colors.greenStart
                }
                GradientStop {
                    position: 0.5
                    color: colors.greenEnd
                }
                GradientStop {
                    position: 1.0
                    color: colors.yellow
                }
            }
        }
    }
}
