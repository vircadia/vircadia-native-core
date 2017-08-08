//
//  MicBar.qml
//  qml/hifi/audio
//
//  Created by Zach Pomerantz on 6/14/2017
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import QtQuick 2.5
import QtQuick.Controls 1.4
import QtQuick.Layouts 1.3
import QtGraphicalEffects 1.0

Rectangle {
    readonly property var level: Audio.inputLevel;

    property bool standalone: false;
    property var dragTarget: null;

    width: 240;
    height: 50;

    radius: 5;

    color: "#00000000";
    border {
        width: mouseArea.containsMouse || mouseArea.containsPress ? 2 : 0;
        color: colors.border;
    }

    // borders are painted over fill, so reduce the fill to fit inside the border
    Rectangle {
        color: standalone ? colors.fill : "#00000000";
        width: 236;
        height: 46;

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
        onClicked: { Audio.muted = !Audio.muted; }
        drag.target: dragTarget;
    }

    QtObject {
        id: colors;

        readonly property string unmuted: "#FFF";
        readonly property string muted: "#E2334D";
        readonly property string gutter: "#575757";
        readonly property string greenStart: "#39A38F";
        readonly property string greenEnd: "#1FC6A6";
        readonly property string red: colors.muted;
        readonly property string fill: "#55000000";
        readonly property string border: standalone ? "#80FFFFFF" : "#55FFFFFF";
        readonly property string icon: Audio.muted ? muted : unmuted;
    }

    Item {
        id: icon;

        anchors {
            left: parent.left;
            leftMargin: 5;
            verticalCenter: parent.verticalCenter;
        }

        width: 40;
        height: 40;

        Item {
            Image {
                readonly property string unmutedIcon: "../../../icons/tablet-icons/mic-unmute-i.svg";
                readonly property string mutedIcon: "../../../icons/tablet-icons/mic-mute-i.svg";

                id: image;
                source: Audio.muted ? mutedIcon : unmutedIcon;

                width: 30;
                height: 30;
                anchors {
                    left: parent.left;
                    leftMargin: 5;
                    top: parent.top;
                    topMargin: 5;
                }
            }

            ColorOverlay {
                anchors { fill: image }
                source: image;
                color: colors.icon;
            }
        }
    }

    Item {
        id: status;

        readonly property string color: Audio.muted ? colors.muted : colors.unmuted;

        visible: Audio.muted;

        anchors {
            left: parent.left;
            leftMargin: 50;
            verticalCenter: parent.verticalCenter;
        }

        width: 170;
        height: 8

        Text {
            anchors {
                horizontalCenter: parent.horizontalCenter;
                verticalCenter: parent.verticalCenter;
            }

            color: parent.color;

            text: Audio.muted ? "MUTED" : "MUTE";
            font.pointSize: 12;
        }

        Rectangle {
            anchors {
                left: parent.left;
                verticalCenter: parent.verticalCenter;
            }

            width: 50;
            height: 4;
            color: parent.color;
        }

        Rectangle {
            anchors {
                right: parent.right;
                verticalCenter: parent.verticalCenter;
            }

            width: 50;
            height: 4;
            color: parent.color;
        }
    }

    Item {
        id: bar;

        visible: !status.visible;

        anchors.fill: status;

        width: status.width;

        Rectangle { // base
            radius: 4;
            anchors { fill: parent }
            color: colors.gutter;
        }

        Rectangle { // mask
            id: mask;
            width: parent.width * level;
            radius: 5;
            anchors {
                bottom: parent.bottom;
                bottomMargin: 0;
                top: parent.top;
                topMargin: 0;
                left: parent.left;
                leftMargin: 0;
            }
        }

        LinearGradient {
            anchors { fill: mask }
            source: mask
            start: Qt.point(0, 0);
            end: Qt.point(170, 0);
            gradient: Gradient {
                GradientStop {
                    position: 0;
                    color: colors.greenStart;
                }
                GradientStop {
                    position: 0.8;
                    color: colors.greenEnd;
                }
                GradientStop {
                    position: 0.81;
                    color: colors.red;
                }
                GradientStop {
                    position: 1;
                    color: colors.red;
                }
            }
        }
    }
}
