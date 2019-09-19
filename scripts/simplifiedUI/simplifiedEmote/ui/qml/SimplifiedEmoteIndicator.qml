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
import hifi.simplifiedUI.simplifiedControls 1.0 as SimplifiedControls

Rectangle {
    id: root
    color: simplifiedUI.colors.white
    anchors.fill: parent
    property int originalWidth: 48
    property int expandedWidth: mainEmojiContainer.width + drawerContainer.width
    // For the below to work, the Repeater's Item's second child must be the individual button's `MouseArea`
    property int requestedWidth: (
        root.allowEmoteDrawerExpansion && (
            drawerContainer.keepDrawerExpanded ||
            emoteIndicatorMouseArea.containsMouse ||
            emoteButtonsRepeater.itemAt(0).hovered ||
            emoteButtonsRepeater.itemAt(1).hovered ||
            emoteButtonsRepeater.itemAt(2).hovered ||
            emoteButtonsRepeater.itemAt(3).hovered ||
            emoteButtonsRepeater.itemAt(4).hovered ||
            emoteButtonsRepeater.itemAt(5).hovered)
        ) ? expandedWidth : originalWidth;
    readonly property int totalEmojiDurationMS: 7000 // Must match `TOTAL_EMOJI_DURATION_MS` in `simplifiedEmoji.js`
    readonly property string emoteIconSource: "images/emote_Icon.svg"
    property bool allowEmoteDrawerExpansion: Settings.getValue("simplifiedUI/allowEmoteDrawerExpansion", true)


    onRequestedWidthChanged: {
        root.requestNewWidth(root.requestedWidth);
    }

    Behavior on requestedWidth {
        enabled: false // Set this to `true` once we have a different windowing system that better supports on-screen widgets
                       // like the Emote Indicator.
        SmoothedAnimation { duration: 220 }
    }

    Connections {
        target: Settings

        onValueChanged: {
            if (setting === "simplifiedUI/allowEmoteDrawerExpansion") {
                root.allowEmoteDrawerExpansion = value;
            }
        }
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
            id: emoteIndicatorLowOpacity
            width: emoteIndicator.width
            height: emoteIndicator.height
            anchors.centerIn: parent
            source: emoteIndicator.source
            opacity: 0.5
            fillMode: Image.PreserveAspectFit
            // All "reactions" have associated icon filenames that contain "Icon.svg"; emojis don't.
            visible: emoteIndicator.source.toString().indexOf("Icon.svg") === -1
            mipmap: true
        }

        Image {
            id: emoteIndicator
            width: 30
            height: 30
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.verticalCenter: parent.verticalCenter
            source: root.emoteIconSource
            mipmap: true
            visible: false

            onSourceChanged: {
                // All "reactions" have associated icon filenames that contain "Icon.svg"; emojis don't.
                progressCircle.endAnimation = false;
                progressCircle.arcEnd = 360;
                progressCircle.endAnimation = true;

                var sourceIsEmojiImage = source.toString().indexOf("Icon.svg") === -1;

                // This kicks off the progress circle animation
                if (sourceIsEmojiImage) {
                    progressCircle.arcEnd = 0;
                    restoreEmoteIconTimer.restart();
                } else {
                    restoreEmoteIconTimer.stop();
                }
            }
        }

        Timer {
            id: restoreEmoteIconTimer
            running: false
            repeat: false
            interval: root.totalEmojiDurationMS
            onTriggered: {
                emoteIndicator.source = root.emoteIconSource;
            }
        }

        // The overlay used during the pie timeout
        SimplifiedControls.ProgressCircle {
            id: progressCircle
            animationDuration: root.totalEmojiDurationMS
            anchors.centerIn: emoteIndicator
            size: emoteIndicator.width * 2
            opacity: 0.5
            colorCircle: "#FFFFFF"
            colorBackground: "#E6E6E6"
            showBackground: false
            isPie: true
            arcBegin: 0
            arcEnd: 360
            visible: false
        }

        OpacityMask {
            anchors.fill: emoteIndicator
            source: emoteIndicator
            maskSource: progressCircle
        }

        ColorOverlay {
            id: emoteIndicatorColorOverlay
            anchors.fill: emoteIndicator
            source: emoteIndicator
            color: "#ffffff"
            // All "reactions" have associated icon filenames that contain "Icon.svg"; emojis don't.
            opacity: emoteIndicator.source.toString().indexOf("Icon.svg") > -1 ? 1.0 : 0.0
        }

        Image {
            id: lockIcon
            width: 12
            height: 12
            anchors.top: parent.top
            anchors.topMargin: 2
            anchors.left: parent.left
            anchors.leftMargin: 0
            source: "images/lock_Icon.svg"
            fillMode: Image.PreserveAspectFit
            mipmap: true
            visible: false
        }

        ColorOverlay {
            id: lockIconColorOverlay
            anchors.fill: lockIcon
            source: lockIcon
            color: "#ffffff"
            visible: root.allowEmoteDrawerExpansion && drawerContainer.keepDrawerExpanded
        }

        MouseArea {
            id: emoteIndicatorMouseArea
            anchors.fill: parent
            hoverEnabled: enabled

            onClicked: {
                Tablet.playSound(TabletEnums.ButtonClick);
                drawerContainer.keepDrawerExpanded = !drawerContainer.keepDrawerExpanded;
                // If the drawer is no longer expanded, disable this MouseArea (which will close
                // the emote tray) until the user's cursor leaves the MouseArea (see `onExited()` below).
                if (!drawerContainer.keepDrawerExpanded) {
                    emoteIndicatorMouseArea.enabled = false;
                }
            }

            onEntered: {
                Tablet.playSound(TabletEnums.ButtonHover);
            }

            onExited: {
                emoteIndicatorMouseArea.enabled = true;
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
                ListElement { imageURL: "images/positive_Icon.svg"; hotkey: "Z"; method: "positive" }
                ListElement { imageURL: "images/negative_Icon.svg"; hotkey: "X"; method: "negative" }
                ListElement { imageURL: "images/applaud_Icon.svg"; hotkey: "C"; method: "applaud" }
                ListElement { imageURL: "images/raiseHand_Icon.svg"; hotkey: "V"; method: "raiseHand" }
                ListElement { imageURL: "images/point_Icon.svg"; hotkey: "B"; method: "point" }
                ListElement { imageURL: "images/emoji_Icon.svg"; hotkey: "F"; method: "toggleEmojiApp" }
            }

            Rectangle {
                width: mainEmojiContainer.width
                height: drawerContainer.height
                // For the below to work, the This Rectangle's second child must be the `MouseArea`
                color: hovered ? "#000000" : simplifiedUI.colors.white
                property alias hovered: emoteTrayMouseArea.containsMouse

                Image {
                    id: emoteTrayButtonImage
                    width: 30
                    height: 30
                    anchors.horizontalCenter: parent.horizontalCenter
                    anchors.verticalCenter: parent.verticalCenter
                    source: model.imageURL
                    mipmap: true
                    visible: false
                }

                ColorOverlay {
                    anchors.fill: emoteTrayButtonImage
                    source: emoteTrayButtonImage
                    color: parent.hovered ? "#ffffff" : "#000000"
                }

                Rectangle {
                    visible: parent.hovered
                    anchors.left: parent.left
                    anchors.bottom: parent.bottom
                    width: toolTipText.width + 4
                    height: toolTipText.height - 3
                    color: "#000000"
                    opacity: 0.8
                    radius: 4

                    HifiStylesUit.GraphikSemiBold {
                        id: toolTipText
                        anchors.left: parent.left
                        anchors.leftMargin: 2
                        anchors.bottom: parent.bottom
                        width: paintedWidth
                        height: paintedHeight
                        text: model.hotkey
                        verticalAlignment: TextInput.AlignBottom
                        horizontalAlignment: TextInput.AlignLeft
                        color: simplifiedUI.colors.text.white
                        size: 20
                    }
                }

                MouseArea {
                    id: emoteTrayMouseArea
                    anchors.fill: parent
                    hoverEnabled: true

                    onEntered: {
                        Tablet.playSound(TabletEnums.ButtonHover);
                    }

                    onPressed: {
                        Tablet.playSound(TabletEnums.ButtonClick);
                        sendToScript({
                            "source": "EmoteAppBar.qml",
                            "method": model.method,
                            "data": { "isPressingAndHolding": true }
                        });
                    }

                    onReleased: {
                        sendToScript({
                            "source": "EmoteAppBar.qml",
                            "method": model.method,
                            "data": { "isPressingAndHolding": false }
                        });
                    }

                    onExited: {
                        if (pressed) {
                            sendToScript({
                                "source": "EmoteAppBar.qml",
                                "method": model.method,
                                "data": { "isPressingAndHolding": false }
                            });
                        }
                    }
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
                if (message.data.iconURL) {
                    emoteIndicator.source = message.data.iconURL;
                } else {
                    console.log("SimplifiedEmoteIndicator.qml: Error! `updateEmoteIndicator()` called without a new `iconURL`!");
                }
                break;

            default:
                console.log('SimplifiedEmoteIndicator.qml: Unrecognized message from JS');
                break;
        }
    }

    signal sendToScript(var message);
    signal requestNewWidth(int newWidth);
}
