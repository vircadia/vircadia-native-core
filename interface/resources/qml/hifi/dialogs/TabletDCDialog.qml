//
//  TabletDCDialog.qml
//
//  Created by Vlad Stelmahovsky on  3/15/17
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import QtQuick 2.5
import QtQuick.Controls 1.4
import Qt.labs.settings 1.0

import "../../styles-uit"
import "../../controls-uit" as HifiControls
import "../../windows"

Rectangle {
    id: root
    objectName: "DCConectionTiming"

    signal sendToScript(var message);
    property bool isHMD: false

    color: hifi.colors.baseGray

    property int colorScheme: hifi.colorSchemes.dark

    HifiConstants { id: hifi }

    Component.onCompleted: DCModel.refresh()

    Row {
        id: header
        anchors.top: parent.top
        anchors.topMargin: hifi.dimensions.tabletMenuHeader
        anchors.leftMargin: 5
        anchors.rightMargin: 5
        anchors.left: parent.left
        anchors.right: parent.right

        HifiControls.Label {
            id: nameButton
            text: qsTr("Name")
            size: 15
            color: "white"
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            height: 40
            width: 175
        }
        HifiControls.Label {
            id: tsButton
            text: qsTr("Timestamp\n(ms)")
            size: 15
            color: "white"
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            height: 40
            width: 125
        }
        HifiControls.Label {
            id: deltaButton
            text: qsTr("Delta\n(ms)")
            size: 15
            color: "white"
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            height: 40
            width: 80
        }
        HifiControls.Label {
            id: elapseButton
            text: qsTr("Time elapsed\n(ms)")
            size: 15
            color: "white"
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            height: 40
            width: 80
        }
    }

    ListView {
        anchors.leftMargin: 5
        anchors.rightMargin: 5
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: header.bottom
        anchors.topMargin: 5
        anchors.bottom: refreshButton.top
        anchors.bottomMargin: 10
        clip: true
        snapMode: ListView.SnapToItem

        model: DCModel

        delegate: Rectangle {
            anchors.left: parent.left
            anchors.right: parent.right
            height: 30
            color: index % 2 === 0 ? hifi.colors.baseGray : hifi.colors.lightGray
            Row {
                anchors.fill: parent
                spacing: 5
                HifiControls.Label {
                    size: 15
                    text: name
                    color: "white"
                    anchors.verticalCenter: parent.verticalCenter
                    colorScheme: root.colorScheme
                    width: nameButton.width
                }
                HifiControls.Label {
                    size: 15
                    text: timestamp
                    color: "white"
                    anchors.verticalCenter: parent.verticalCenter
                    colorScheme: root.colorScheme
                    horizontalAlignment: Text.AlignHCenter
                    width: tsButton.width
                }
                HifiControls.Label {
                    size: 15
                    text: delta
                    color: "white"
                    anchors.verticalCenter: parent.verticalCenter
                    colorScheme: root.colorScheme
                    horizontalAlignment: Text.AlignHCenter
                    width: deltaButton.width
                }
                HifiControls.Label {
                    size: 15
                    text: timeelapsed
                    color: "white"
                    anchors.verticalCenter: parent.verticalCenter
                    colorScheme: root.colorScheme
                    horizontalAlignment: Text.AlignHCenter
                    width: elapseButton.width
                }
            }
        }
    }

    HifiControls.Button {
        id: refreshButton
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        text: qsTr("Refresh")
        color: hifi.buttons.blue
        colorScheme: root.colorScheme
        height: 30
        onClicked: {
            DCModel.refresh()
        }
    }
}
