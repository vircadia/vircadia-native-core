//
//  EmoteAppBar.qml
//
//  Created by Zach Fox on 2019-07-08
//  Copyright 2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import QtQuick 2.10
import QtQuick.Controls 2.3
import "../../simplifiedConstants" as SimplifiedConstants
import "../../simplifiedControls" as SimplifiedControls
import stylesUit 1.0 as HifiStylesUit
import TabletScriptingInterface 1.0

Rectangle {
    id: root
    color: simplifiedUI.colors.white
    anchors.fill: parent

    property int originalWidth: 48
    property int hoveredWidth: 480
    property int requestedWidth

    onRequestedWidthChanged: {
        root.requestNewWidth(root.requestedWidth);
    }

    Behavior on requestedWidth {
        enabled: true
        SmoothedAnimation { duration: 220 }
    }

    SimplifiedConstants.SimplifiedConstants {
        id: simplifiedUI
    }

    MouseArea {
        anchors.fill: parent
        hoverEnabled: enabled
        onEntered: {
            Tablet.playSound(TabletEnums.ButtonHover);
            root.requestedWidth = root.hoveredWidth;
        }
        onExited: {
            Tablet.playSound(TabletEnums.ButtonClick);
            root.requestedWidth = root.originalWidth;
        }
    }

    Rectangle {
        id: mainEmojiContainer
        z: 2
        color: simplifiedUI.colors.darkBackground
        anchors.top: parent.top
        anchors.left: parent.left
        height: parent.height
        width: root.originalWidth

        HifiStylesUit.GraphikRegular {
            text: "😊"
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.verticalCenter: parent.verticalCenter
            anchors.horizontalCenterOffset: -2
            anchors.verticalCenterOffset: -2
            horizontalAlignment: Text.AlignHCenter
            size: 26
            color: simplifiedUI.colors.text.almostWhite
        }
    }

    Rectangle {
        id: drawerContainer
        z: 1
        color: simplifiedUI.colors.white
        anchors.top: parent.top
        anchors.right: parent.right
        height: parent.height
        width: root.hoveredWidth

        HifiStylesUit.GraphikRegular {
            text: "Emotes go here."
            anchors.fill: parent
            horizontalAlignment: Text.AlignHCenter
            size: 20
            color: simplifiedUI.colors.text.black
        }
    }

    function fromScript(message) {
        switch (message.method) {
            default:
                console.log('EmoteAppBar.qml: Unrecognized message from JS');
                break;
        }
    }
    signal sendToScript(var message);
    signal requestNewWidth(int newWidth);
}
