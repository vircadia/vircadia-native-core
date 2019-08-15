//
//  SimplifiedEmoteIndicator.qml
//
//  Created by Milad Nazeri on 2019-08-05
//  Based on work from Zach Fox on 2019-07-08
//  Copyright 2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import QtQuick 2.10
import QtQuick.Controls 2.3
import stylesUit 1.0 as HifiStylesUit
import TabletScriptingInterface 1.0
import hifi.simplifiedUI.simplifiedConstants 1.0 as SimplifiedConstants

Rectangle {
    id: root
    color: simplifiedUI.colors.white
    anchors.fill: parent

    property int originalWidth: 48
    property int expandedWidth: mainEmojiContainer.width + drawerContainer.width
    // For the below to work, the Repeater's Item's second child must be the individual button's `MouseArea`
    property int requestedWidth: (drawerContainer.keepDrawerExpanded ||
        emoteIndicatorMouseArea.containsMouse ||
        emoteButtonsRepeater.itemAt(0).children[1].containsMouse ||
        emoteButtonsRepeater.itemAt(1).children[1].containsMouse ||
        emoteButtonsRepeater.itemAt(2).children[1].containsMouse ||
        emoteButtonsRepeater.itemAt(3).children[1].containsMouse ||
        emoteButtonsRepeater.itemAt(4).children[1].containsMouse ||
        emoteButtonsRepeater.itemAt(5).children[1].containsMouse) ? expandedWidth : originalWidth;

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

    Rectangle {
        id: mainEmojiContainer
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
            color: simplifiedUI.colors.white
        }

        MouseArea {
            id: emoteIndicatorMouseArea
            anchors.fill: parent
            hoverEnabled: enabled

            onClicked: {
                Tablet.playSound(TabletEnums.ButtonClick);
                drawerContainer.keepDrawerExpanded = !drawerContainer.keepDrawerExpanded;
            }

            onEntered: {
                Tablet.playSound(TabletEnums.ButtonHover);
            }
        }
    }

    Row {
        id: drawerContainer
        property bool keepDrawerExpanded: false
        anchors.top: parent.top
        anchors.left: mainEmojiContainer.right
        height: parent.height
        width: childrenRect.width

        Repeater {
            id: emoteButtonsRepeater
            model: ListModel {
                id: buttonsModel
                ListElement { text: "Z"; method: "happyPressed" }
                ListElement { text: "C"; method: "sadPressed" }
                ListElement { text: "V"; method: "raiseHandPressed" }
                ListElement { text: "B"; method: "applaudPressed" }
                ListElement { text: "N"; method: "pointPressed" }
                ListElement { text: "😊"; method: "toggleEmojiApp" }
            }

            Rectangle {
                width: mainEmojiContainer.width
                height: drawerContainer.height
                // For the below to work, the This Rectangle's second child must be the `MouseArea`
                color: children[1].containsMouse ? "#CCCCCC" : simplifiedUI.colors.white

                HifiStylesUit.GraphikRegular {
                    text: model.text
                    // Gotta special-case the below for the emoji button, or else it looks off-center.
                    anchors.fill: text === "😊" ? undefined : parent
                    anchors.horizontalCenter: text === "😊" ? parent.horizontalCenter : undefined
                    anchors.verticalCenter: text === "😊" ? parent.verticalCenter : undefined
                    anchors.horizontalCenterOffset: text === "😊" ? -2 : undefined
                    anchors.verticalCenterOffset: text === "😊" ? -2 : undefined
                    horizontalAlignment: Text.AlignHCenter
                    size: 26
                    color: simplifiedUI.colors.text.black
                }

                MouseArea {
                    anchors.fill: parent
                    hoverEnabled: true
                    onClicked: {
                        Tablet.playSound(TabletEnums.ButtonClick);
                        sendToScript({
                            "source": "EmoteAppBar.qml",
                            "method": model.method
                        });
                    }

                    onEntered: {
                        Tablet.playSound(TabletEnums.ButtonHover);
                    }
                }
            }
        }
    }

    signal sendToScript(var message);
    signal requestNewWidth(int newWidth);
}
