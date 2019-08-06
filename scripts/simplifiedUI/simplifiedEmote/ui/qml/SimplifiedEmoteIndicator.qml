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
/*
    Not sure if there is a better way to request this?
*/
import "qrc:////qml//hifi//simplifiedUI//simplifiedConstants" as SimplifiedConstants

Rectangle {
    id: root
    color: simplifiedUI.colors.darkBackground
    anchors.fill: parent

    property int originalWidth: 48
    property int hoveredWidth: 357
    property int requestedWidth

    property bool overEmoteButton: false
    property bool overEmojiButton: false

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

    /*
        MILAD NOTE: 
        Right now there are some strange issues around overlapping
        onEnters with mouse areas.  I was having a hard time getting an individual
        hover happening.  Not sure if there is just one mouse menu that gives 
        info about what is underneath to be able to be toggle an individual icon on and off.
    */
    MouseArea {
        id: emoteMouseArea
        anchors.fill: parent
        hoverEnabled: enabled
        propagateComposedEvents: true;
        onEntered: {
            Tablet.playSound(TabletEnums.ButtonHover);
            root.requestedWidth = root.hoveredWidth;
            emojiMouseArea.hoverEnabled = true;
            root.overEmoteButton = true;
        }
        onExited: {
            root.overEmoteButton = false;
            Tablet.playSound(TabletEnums.ButtonClick);
            root.requestedWidth = root.originalWidth;
        }
        z: 2
    }

    /*
        MILAD NOTE:
        This would be where you would swap the current activiated icon
    */
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

    /*
        MILAD NOTE:
        The main container of the actual buttons
    */
    Rectangle {
        id: drawerContainer
        z: 1
        color: simplifiedUI.colors.white
        anchors.top: parent.top
        anchors.right: parent.right
        height: parent.height
        width: root.hoveredWidth

        /*
            MILAD NOTE:
            Below is one button.  They are just text representations now, but
            they probably be switched with Image {} to be supplied by Joshua.
        */

        Rectangle {
            id: happyBackground
            z: 3
            anchors.leftMargin: root.originalWidth + 3;
            width: root.originalWidth
            height: parent.height
            anchors.left: parent.left
            color: simplifiedUI.colors.darkBackground

            HifiStylesUit.GraphikRegular {
                id: happyButton
                text: "Z"
                z: 3
                anchors.fill: parent
                anchors.verticalCenter: parent.verticalCenter
                anchors.rightMargin: 1
                anchors.verticalCenterOffset: -2
                horizontalAlignment: Text.AlignHCenter
                size: 26
                color: simplifiedUI.colors.text.almostWhite
            }
            MouseArea {
                id: happyMouseArea
                anchors.fill: parent
                hoverEnabled: false
                propagateComposedEvents: true;
                z: 4
                onClicked: {
                    console.log("GOT THE CLICK");
                    sendToScript({
                        "source": "EmoteAppBar.qml",
                        "method": "happyPressed"
                    });
                }
                onEntered: {
                }
            }
        }

        Rectangle {
            id: sadBackground
            z: 3
            width: root.originalWidth
            height: parent.height
            anchors.left: happyBackground.right
            anchors.leftMargin: 3
            color: simplifiedUI.colors.darkBackground

            HifiStylesUit.GraphikRegular {
                id: sadButton
                text: "C"
                z: 3
                anchors.fill: parent
                anchors.verticalCenter: parent.verticalCenter
                anchors.rightMargin: 1
                anchors.verticalCenterOffset: -2
                horizontalAlignment: Text.AlignHCenter
                size: 26
                color: simplifiedUI.colors.text.almostWhite
            }
            MouseArea {
                id: sadMouseArea
                anchors.fill: parent
                hoverEnabled: false
                propagateComposedEvents: true;
                z: 4
                onClicked: {
                    console.log("GOT THE CLICK");
                    sendToScript({
                        "source": "EmoteAppBar.qml",
                        "method": "sadPressed"
                    });
                }
                onEntered: {
                }
            }
        }

        Rectangle {
            id: raiseHandBackground
            z: 3
            width: root.originalWidth
            height: parent.height
            anchors.left: sadBackground.right
            anchors.leftMargin: 3
            color: simplifiedUI.colors.darkBackground

            HifiStylesUit.GraphikRegular {
                id: raiseHandButton
                text: "V"
                z: 3
                anchors.fill: parent
                anchors.verticalCenter: parent.verticalCenter
                anchors.rightMargin: 1
                anchors.verticalCenterOffset: -2
                horizontalAlignment: Text.AlignHCenter
                size: 26
                color: simplifiedUI.colors.text.almostWhite
            }
            MouseArea {
                id: raiseHandMouseArea
                anchors.fill: parent
                hoverEnabled: false
                propagateComposedEvents: true;
                z: 4
                onClicked: {
                    console.log("GOT THE CLICK");
                    sendToScript({
                        "source": "EmoteAppBar.qml",
                        "method": "raiseHandPressed"
                    });
                }
                onEntered: {
                }
            }
        }

        Rectangle {
            id: clapBackground
            z: 3
            width: root.originalWidth
            height: parent.height
            anchors.left: raiseHandBackground.right
            anchors.leftMargin: 3
            color: simplifiedUI.colors.darkBackground

            HifiStylesUit.GraphikRegular {
                id: clapButton
                text: "B"
                z: 3
                anchors.fill: parent
                anchors.verticalCenter: parent.verticalCenter
                anchors.rightMargin: 1
                anchors.verticalCenterOffset: -2
                horizontalAlignment: Text.AlignHCenter
                size: 26
                color: simplifiedUI.colors.text.almostWhite
            }
            MouseArea {
                id: clapMouseArea
                anchors.fill: parent
                hoverEnabled: false
                propagateComposedEvents: true;
                z: 4
                onClicked: {
                    console.log("GOT THE CLICK");
                    sendToScript({
                        "source": "EmoteAppBar.qml",
                        "method": "applaudPressed"
                    });
                }
                onEntered: {
                }
            }
        }

        Rectangle {
            id: pointBackground
            z: 3
            width: root.originalWidth
            height: parent.height
            anchors.left: clapBackground.right
            anchors.leftMargin: 3
            color: simplifiedUI.colors.darkBackground

            HifiStylesUit.GraphikRegular {
                id: pointButton
                text: "N"
                z: 3
                anchors.fill: parent
                anchors.verticalCenter: parent.verticalCenter
                anchors.rightMargin: 1
                anchors.verticalCenterOffset: -2
                horizontalAlignment: Text.AlignHCenter
                size: 26
                color: simplifiedUI.colors.text.almostWhite
            }
            MouseArea {
                id: pointMouseArea
                anchors.fill: parent
                hoverEnabled: false
                propagateComposedEvents: true;
                z: 4
                onClicked: {
                    console.log("GOT THE CLICK");
                    sendToScript({
                        "source": "EmoteAppBar.qml",
                        "method": "pointPressed"
                    });
                }
                onEntered: {
                }
            }
        }

        Rectangle {
            id: emojiButtonBackground
            z: 3
            width: root.originalWidth
            height: parent.height
            anchors.right: parent.right
            anchors.rightMargin: 3
            color: simplifiedUI.colors.darkBackground

            HifiStylesUit.GraphikRegular {
                id: emojiButton
                text: "😊"
                z: 3
                anchors.fill: parent
                anchors.verticalCenter: parent.verticalCenter
                anchors.rightMargin: 1
                anchors.verticalCenterOffset: -2
                horizontalAlignment: Text.AlignHCenter
                size: 26
                color: simplifiedUI.colors.text.almostWhite
            }
            MouseArea {
                id: emojiMouseArea
                anchors.fill: parent
                hoverEnabled: false
                propagateComposedEvents: true;
                z: 4
                onClicked: {
                    console.log("GOT THE CLICK");
                    sendToScript({
                        "source": "EmoteAppBar.qml",
                        "method": "toggleEmojiApp"
                    });
                }
                onEntered: {
                }
            }
        }
    }

    signal sendToScript(var message);
    signal requestNewWidth(int newWidth);
}
