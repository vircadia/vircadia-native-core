//
//  Prop/style/PiFoldedPanel.qml
//
//  Created by Sam Gateau on 4/17/2019
//  Copyright 2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0.html
//

import QtQuick 2.7

Item {
    Global { id: global }
    id: root

    property var label: "panel"

    property alias isUnfold: headerFolderIcon.icon
    property alias hasContent: headerFolderIcon.filled
    property var indentDepth: 0

    // Panel Header Data Component
    property Component panelHeaderData: defaultPanelHeaderData  
    Component { // default is a Label
        id: defaultPanelHeaderData
        PiLabel {
            text: root.label
            horizontalAlignment: Text.AlignHCenter
        }
    }

    // Panel Frame Data Component
    property Component panelFrameData: defaultPanelFrameData  
    Component { // default is a column
        id: defaultPanelFrameData
        Column {
        }
    }

    property alias panelFrameContent: _panelFrameData.item

    // Header Item
    Rectangle {
        id: header
        height: global.slimHeight
        anchors.left: parent.left           
        anchors.right: parent.right
        anchors.margins: 0

        color: global.colorBackShadow // header of group is darker

        // First in the header, some indentation spacer
        Item {
            id: indentSpacer
            width: (headerFolderIcon.width * root.indentDepth) + global.horizontalMargin  // Must be non-zero
            height: parent.height

            anchors.verticalCenter: parent.verticalCenter
        }

        // Second, the folder button / indicator
        Item {
            id: headerFolder
            anchors.left: indentSpacer.right
            width: headerFolderIcon.width * 2
            anchors.verticalCenter: header.verticalCenter
            height: parent.height
            
            PiCanvasIcon {
                id: headerFolderIcon
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.verticalCenter: parent.verticalCenter
                fillColor: global.colorOrangeAccent 
                iconMouseArea.onClicked: { root.isUnfold = !root.isUnfold }
            }
        }

        // Next the header container
        // by default containing a Label showing the root.label
        Loader { 
            sourceComponent: panelHeaderData
            anchors.left: headerFolder.right
            anchors.right: header.right
            anchors.rightMargin: global.horizontalMargin * 2
            anchors.verticalCenter: header.verticalCenter
            height: parent.height
        }
    }

    // The Panel container 
    Rectangle {
        id: frame
        visible: root.isUnfold

        color: "transparent"
        border.color: global.colorBorderLight
        border.width: global.valueBorderWidth
        radius: global.valueBorderRadius

        anchors.margins: 0
        anchors.left: parent.left           
        anchors.right: parent.right
        anchors.top: header.bottom
        anchors.bottom: root.bottom  

        // Next the panel frame data
        Loader { 
            id: _panelFrameData
            sourceComponent: panelFrameData
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.leftMargin: 0
            anchors.rightMargin: 0
            clip: true
        }
    }

    height: header.height + isUnfold * _panelFrameData.item.height
    anchors.margins: 0
    anchors.left: parent.left           
    anchors.right: parent.right
}