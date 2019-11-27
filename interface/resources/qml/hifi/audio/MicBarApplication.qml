//
//  MicBarApplication.qml
//  qml/hifi/audio
//
//  Created by Zach Pomerantz on 6/14/2017
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import QtQuick 2.5
import QtGraphicalEffects 1.0

import stylesUit 1.0
import TabletScriptingInterface 1.0

Rectangle {
    id: micBar;
    readonly property var level: AudioScriptingInterface.inputLevel;
    readonly property var clipping: AudioScriptingInterface.clipping;
    property var muted: AudioScriptingInterface.muted;
    property var pushToTalk: AudioScriptingInterface.pushToTalk;
    property var pushingToTalk: AudioScriptingInterface.pushingToTalk;
    readonly property var userSpeakingLevel: 0.4;
    property bool gated: false;

    Timer {
        // used to hold the muted warning.
        id: mutedTimer

        interval: 2000;
        running: false;
        repeat: false;
        property bool isRunning: false;
        onTriggered: {
            isRunning = false;
        }
    }

    Component.onCompleted: {
        AudioScriptingInterface.noiseGateOpened.connect(function() { micBar.gated = false; });
        AudioScriptingInterface.noiseGateClosed.connect(function() { micBar.gated = true; });
        HMD.displayModeChanged.connect(function() {
            muted = AudioScriptingInterface.muted;
            pushToTalk = AudioScriptingInterface.pushToTalk;
        });
        AudioScriptingInterface.mutedChanged.connect(function() {
            muted = AudioScriptingInterface.muted;
        });
        AudioScriptingInterface.pushToTalkChanged.connect(function() {
            pushToTalk = AudioScriptingInterface.pushToTalk;
        });
    }

    readonly property string unmutedIcon: "../../../icons/tablet-icons/mic-unmute-i.svg";
    readonly property string mutedIcon: "../../../icons/tablet-icons/mic-mute-i.svg";
    readonly property string pushToTalkIcon: "../../../icons/tablet-icons/mic-ptt-i.svg";
    readonly property string clippingIcon: "../../../icons/tablet-icons/mic-clip-i.svg";
    readonly property string gatedIcon: "../../../icons/tablet-icons/mic-gate-i.svg";
    property bool standalone: false;
    property var dragTarget: null;

    width: 44;
    height: 44;

    radius: 5;
    opacity: 0.7;

    onLevelChanged: {
        var mutedAndSpeaking = (muted && (level >= userSpeakingLevel));
        if (!mutedTimer.isRunning && !pushToTalk) {
            if (mutedAndSpeaking) {
                mutedTimer.start();
                mutedTimer.isRunning = true;
                statusText.text = "MUTED";
            } else {
                statusText.text = "";
            }
        }
        var rectOpacity = mutedAndSpeaking ? 1.0 : 0.7;
        if (pushToTalk && !pushingToTalk) {
            rectOpacity = (mouseArea.containsMouse) ? 1.0 : 0.7;
        } else if (mouseArea.containsMouse && rectOpacity != 1.0) {
            rectOpacity = 1.0;
        }
        micBar.opacity = rectOpacity;
    }

    onPushToTalkChanged: {
        statusText.text = pushToTalk ? HMD.active ? "PTT" : "PTT-(T)" : "";
    }

    color: "#00000000";
    border {
        width: mouseArea.containsMouse || mouseArea.containsPress ? 2 : 0;
        color: colors.border;
    }

    // borders are painted over fill, so reduce the fill to fit inside the border
    Rectangle {
        color: standalone ? colors.fill : "#00000000";
        width: 40;
        height: 40;

        radius: 5;

        anchors {
            verticalCenter: parent.verticalCenter;
            horizontalCenter: parent.horizontalCenter;
        }
    }

    MouseArea {
        id: mouseArea;

        anchors {
            left: icon.left;
            right: bar.right;
            top: icon.top;
            bottom: icon.bottom;
        }

        hoverEnabled: true;
        scrollGestureEnabled: false;
        onClicked: {
            if (pushToTalk) {
                return;
            }
            AudioScriptingInterface.muted = !muted;
            Tablet.playSound(TabletEnums.ButtonClick);
            muted = Qt.binding(function() { return AudioScriptingInterface.muted; }); // restore binding
        }
        drag.target: dragTarget;
        onContainsMouseChanged: {
            if (containsMouse) {
                Tablet.playSound(TabletEnums.ButtonHover);
            }
        }
    }

    QtObject {
        id: colors;

        readonly property string unmutedColor: "#FFF";
        readonly property string gatedColor: "#00BDFF";
        readonly property string mutedColor: "#E2334D";
        readonly property string gutter: "#575757";
        readonly property string greenStart: "#39A38F";
        readonly property string greenEnd: "#1FC6A6";
        readonly property string yellow: "#C0C000";
        readonly property string fill: "#55000000";
        readonly property string border: standalone ? "#80FFFFFF" : "#55FFFFFF";
        readonly property string icon: (muted || clipping) ? mutedColor : micBar.gated ? gatedColor : unmutedColor;
    }

    Item {
        id: icon;

        anchors {
            left: parent.left;
            top: parent.top;
        }

        width: 40;
        height: 40;

        Item {
            Image {
                id: image;
                source: (pushToTalk) ? pushToTalkIcon : muted ? mutedIcon :
                    clipping ? clippingIcon : micBar.gated ? gatedIcon : unmutedIcon;
                width: 29;
                height: 32;
                anchors {
                    left: parent.left;
                    top: parent.top;
                    topMargin: 5;
                }
            }

            ColorOverlay {
                id: imageOverlay
                anchors { fill: image }
                source: image;
                color: pushToTalk ? (pushingToTalk ? colors.unmutedColor : colors.mutedColor) : colors.icon;
            }
        }
    }

    Item {
        id: status;

        visible: pushToTalk || (muted && (level >= userSpeakingLevel));

        anchors {
            left: parent.left;
            top: icon.bottom;
            topMargin: 2;
        }

        width: parent.width;
        height: statusTextMetrics.height;

        TextMetrics {
            id: statusTextMetrics
            text: statusText.text
            font: statusText.font
        }

        RalewaySemiBold {
            id: statusText
            anchors {
                horizontalCenter: parent.horizontalCenter;
                verticalCenter: parent.verticalCenter;
            }

            color: pushToTalk ? (pushingToTalk ? colors.unmutedColor : colors.mutedColor) : (level >= userSpeakingLevel && muted) ? colors.mutedColor : colors.unmutedColor;
            font.bold: true

            size: 12;
        }
    }

    Item {
        id: bar;

        anchors {
            right: parent.right;
            rightMargin: 7;
            top: parent.top
            topMargin: 5
        }

        width: 8;
        height: 32;

        Rectangle { // base
            id: baseBar
            radius: 4;
            anchors { fill: parent }
            color: colors.gutter;
        }

        Rectangle { // mask
            id: mask;
            visible: (!(pushToTalk && !pushingToTalk))
            height: parent.height * level;
            width: parent.width;
            radius: 5;
            anchors {
                bottom: parent.bottom;
                bottomMargin: 0;
                left: parent.left;
                leftMargin: 0;
            }
        }

        LinearGradient {
            anchors { fill: mask }
            visible: (!(pushToTalk && !pushingToTalk))
            source: mask
            start: Qt.point(0, 0);
            end: Qt.point(0, bar.height);
            rotation: 180
            gradient: Gradient {
                GradientStop {
                    position: 0.0;
                    color: colors.greenStart;
                }
                GradientStop {
                    position: 0.5;
                    color: colors.greenEnd;
                }
                GradientStop {
                    position: 1.0;
                    color: colors.yellow;
                }
            }
        }
    }
}
