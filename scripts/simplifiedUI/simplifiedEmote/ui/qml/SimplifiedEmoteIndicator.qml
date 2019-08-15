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
import QtGraphicalEffects 1.0
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

        Image {
            id: emoteIndicator
            width: 30
            height: 30
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.verticalCenter: parent.verticalCenter
            source: "images/emote_Icon.svg"
        }

        ColorOverlay {
            anchors.fill: emoteIndicator
            opacity: emoteIndicatorMouseArea.containsMouse ? 1.0 : 0.7
            source: emoteIndicator
            color: "#ffffff"
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
                ListElement { imageURL: "images/happy_Icon.svg"; method: "positive" }
                ListElement { imageURL: "images/sad_Icon.svg"; method: "negative" }
                ListElement { imageURL: "images/raiseHand_Icon.svg"; method: "raiseHand" }
                ListElement { imageURL: "images/clap_Icon.svg"; method: "applaud" }
                ListElement { imageURL: "images/point_Icon.svg"; method: "point" }
                ListElement { imageURL: "images/emote_Icon.svg"; method: "toggleEmojiApp" }
            }

            Rectangle {
                width: mainEmojiContainer.width
                height: drawerContainer.height
                // For the below to work, the This Rectangle's second child must be the `MouseArea`
                color: children[1].containsMouse ? "#000000" : simplifiedUI.colors.white

                Image {
                    id: emoteTrayButtonImage
                    width: 30
                    height: 30
                    anchors.horizontalCenter: parent.horizontalCenter
                    anchors.verticalCenter: parent.verticalCenter
                    source: model.imageURL
                    mipmap: true
                }

                MouseArea {
                    id: emoteTrayMouseArea
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

                ColorOverlay {
                    anchors.fill: emoteTrayButtonImage
                    opacity: emoteTrayMouseArea.containsMouse ? 1.0 : 0
                    source: emoteTrayButtonImage
                    color: "#ffffff"
                }
            }
        }
    }

    function fromScript(message) {
        if (message.source !== "simplifiedEmote.js") {
            return;
        }

        switch (message.method) {
            case "updateEmoteIndicator":
                if (message.data.icon) {
                print("UPDATING EMOTE INDICATOR");
                    emoteIndicator.text = message.data.icon[0];
                }
                break;
            default:
                print("MESSAGE TO THE EMOTE INDICATOR QML RECEIVED: ", JSON.stringify(message));
                console.log('SimplifiedEmoteIndicator.qml: Unrecognized message from JS');
                break;
        }
    }

    signal sendToScript(var message);
    signal requestNewWidth(int newWidth);
}
