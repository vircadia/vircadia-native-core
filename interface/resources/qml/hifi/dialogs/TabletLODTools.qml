//
//  TabletLODTools.qml
//
//  Created by Vlad Stelmahovsky on  3/11/17
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
    objectName: "LODTools"

    signal sendToScript(var message);
    property bool isHMD: false

    color: hifi.colors.baseGray

    property int colorScheme: hifi.colorSchemes.dark

    HifiConstants { id: hifi }

    readonly property real treeScale: 32768; // ~20 miles.. This is the number of meters of the 0.0 to 1.0 voxel universe
    readonly property real halfTreeScale: treeScale / 2;

    // This controls the LOD. Larger number will make smaller voxels visible at greater distance.
    readonly property real defaultOctreeSizeScale: treeScale * 400.0

    Column {
        anchors.margins: 10
        anchors.left: parent.left
        anchors.right: parent.right
        y: hifi.dimensions.tabletMenuHeader //-bgNavBar
        spacing: 20

        HifiControls.Label {
            size: 20
            anchors.left: parent.left
            anchors.right: parent.right
            text: qsTr("You can see...")
            colorScheme: root.colorScheme
        }
        HifiControls.Label {
            id: whatYouCanSeeLabel
            color: "red"
            size: 20
            anchors.left: parent.left
            anchors.right: parent.right
            colorScheme: root.colorScheme
        }
        Row {
            anchors.left: parent.left
            anchors.right: parent.right
            spacing: 10

            HifiControls.Label {
                size: 20
                text: qsTr("Manually Adjust Level of Detail:")
                anchors.verticalCenter: parent.verticalCenter
                colorScheme: root.colorScheme
            }

            HifiControls.CheckBox {
                id: adjustCheckbox
                boxSize: 20
                anchors.verticalCenter: parent.verticalCenter
                onCheckedChanged: LODManager.setAutomaticLODAdjust(!checked);
            }
        }

        HifiControls.Label {
            size: 20
            anchors.left: parent.left
            anchors.right: parent.right
            text: qsTr("Level of Detail:")
            colorScheme: root.colorScheme
        }
        HifiControls.Slider {
            id: slider
            enabled: adjustCheckbox.checked
            anchors.left: parent.left
            anchors.right: parent.right
            minimumValue: 5
            maximumValue: 2000
            value: LODManager.getOctreeSizeScale() / treeScale
            tickmarksEnabled: false
            onValueChanged: {
                LODManager.setOctreeSizeScale(value * treeScale);
                whatYouCanSeeLabel.text = LODManager.getLODFeedbackText()
            }
        }

        HifiControls.Button {
            id: uploadButton
            anchors.left: parent.left
            anchors.right: parent.right
            text: qsTr("Reset")
            color: hifi.buttons.blue
            colorScheme: root.colorScheme
            height: 30
            onClicked: {
                slider.value = defaultOctreeSizeScale/treeScale
                adjustCheckbox.checked = false
                LODManager.setAutomaticLODAdjust(adjustCheckbox.checked);
            }
        }
    }
}

