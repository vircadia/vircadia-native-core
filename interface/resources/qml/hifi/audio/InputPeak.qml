//
//  InputPeak.qml
//  qml/hifi/audio
//
//  Created by Zach Pomerantz on 6/20/2017
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
    property var peak;

    width: 70;
    height: 8;

    color: "transparent";

    Item {
        id: colors;

        readonly property string muted: "#E2334D";
        readonly property string gutter: "#575757";
        readonly property string greenStart: "#39A38F";
        readonly property string greenEnd: "#1FC6A6";
        readonly property string red: colors.muted;
    }

    Text {
        id: status;

        anchors {
            horizontalCenter: parent.horizontalCenter;
            verticalCenter: parent.verticalCenter;
        }

        visible: Audio.muted;
        color: colors.muted;

        text: "MUTED";
        font.pointSize: 10;
    }

    Item {
        id: bar;

        width: parent.width;
        height: parent.height;

        anchors { fill: parent }

        visible: !status.visible;

        Rectangle { // base
            radius: 4;
            anchors { fill: parent }
            color: colors.gutter;
        }

        Rectangle { // mask
            id: mask;
            width: parent.width * peak;
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
            end: Qt.point(70, 0);
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
                    position: 0.801;
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
