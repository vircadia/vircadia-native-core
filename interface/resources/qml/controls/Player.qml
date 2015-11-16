//
//  AddressBarDialog.qml
//
//  Created by Austin Davis on 2015/04/14
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

//import Hifi 1.0
import QtQuick 2.4
import QtQuick.Controls 1.2
import QtQuick.Dialogs 1.2
import QtQuick.Controls.Styles 1.2
import "../styles"

Item {
    id: root

    signal play()
    signal rewind()

    property real duration: 100
    property real position: 50
    property bool isPlaying: false
    implicitHeight: 64
    implicitWidth: 640

    Item {
        anchors.top: parent.top
        anchors.left:  parent.left
        anchors.right: parent.right
        height: root.height / 2
        Text {
            id: labelCurrent
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            anchors.left:  parent.left
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            width: 56
            text: "00:00:00"
        }
        Slider {
            value: root.position / root.duration
            anchors.top: parent.top
            anchors.topMargin: 2
            anchors.bottomMargin: 2
            anchors.bottom: parent.bottom
            anchors.left:  labelCurrent.right
            anchors.leftMargin: 4
            anchors.right:  labelDuration.left
            anchors.rightMargin: 4
        }
        Text {
            id: labelDuration
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            anchors.right: parent.right
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            width: 56
            text: "00:00:00"
        }
    }

    Row {
        anchors.bottom: parent.bottom
        anchors.horizontalCenter: parent.horizontalCenter
        height: root.height / 2;
        ButtonAwesome {
            id: cmdPlay
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            text: isPlaying ? "\uf04c" : "\uf04b"
            width: root.height / 2;
            onClicked: root.play();
        }
        ButtonAwesome {
            id: cmdRewind
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            width: root.height / 2
            text: "\uf04a"
            onClicked: root.rewind();
        }
    }
}
