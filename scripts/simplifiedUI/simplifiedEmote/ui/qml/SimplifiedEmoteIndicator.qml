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
    property int expandedWidth: 336
    property int requestedWidth: (drawerContainer.keepDrawerExpanded ||
        emoteIndicatorMouseArea.containsMouse ||
        happyButtonMouseArea.containsMouse ||
        sadButtonMouseArea.containsMouse ||
        raiseHandButtonMouseArea.containsMouse ||
        applaudButtonMouseArea.containsMouse ||
        pointButtonMouseArea.containsMouse ||
        emojiButtonMouseArea.containsMouse) ? expandedWidth : originalWidth;

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
        leftPadding: root.originalWidth
        height: parent.height
        width: root.expandedWidth

        Rectangle {
            id: happyButtonContainer
            width: root.originalWidth
            height: drawerContainer.height
            color: simplifiedUI.colors.white

            HifiStylesUit.GraphikRegular {
                text: "Z"
                anchors.fill: parent
                anchors.rightMargin: 1
                horizontalAlignment: Text.AlignHCenter
                size: 26
                color: simplifiedUI.colors.text.black
            }

            MouseArea {
                id: happyButtonMouseArea
                anchors.fill: parent

                onClicked: {                hoverEnabled: enabled

                onClicked: {
                    Tablet.playSound(TabletEnums.ButtonClick);
                    sendToScript({
                        "source": "EmoteAppBar.qml",
                        "method": "happyPressed"
                    });
                    parent.color = simplifiedUI.colors.darkBackground
                }

                onEntered: {
                    console.log("ENTERED BUTTON");
                    Tablet.playSound(TabletEnums.ButtonHover);
                    parent.color = simplifiedUI.colors.darkBackground
                }

                onExited: {
                    parent.color = simplifiedUI.colors.white
                }
            }
        }

        Rectangle {
            id: sadButtonContainer
            width: root.originalWidth
            height: drawerContainer.height
            color: simplifiedUI.colors.white

            HifiStylesUit.GraphikRegular {
                text: "C"
                anchors.fill: parent
                anchors.rightMargin: 1
                horizontalAlignment: Text.AlignHCenter
                size: 26
                color: simplifiedUI.colors.text.black
            }

            MouseArea {
                id: sadButtonMouseArea
                anchors.fill: parent
                hoverEnabled
                    Tablet.playSound(TabletEnu: enabled

                onClicked: {
                    Tablet.playSound(TabletEnums.ButtonClick);
                    sendToScript({
                        "source": "EmoteAppBar.qml",
                        "method": "sadPressed"
                    });
                    parent.color = simplifiedUI.colors.darkBackground
                }

                onEntered: {
                    console.log("ENTERED BUTTON");
                    Tablet.playSound(TabletEnums.ButtonHover);
                    parent.color = simplifiedUI.colors.darkBackground
                }

                onExited: {
                    parent.color = simplifiedUI.colors.white
                }
            }
        }

        Rectangle {
            id: raiseHandButtonContainer
            width: root.originalWidth
            height: drawerContainer.height
            color: simplifiedUI.colors.white

            HifiStylesUit.GraphikRegular {
                text: "V"
                anchors.fill: parent
                anchors.rightMargin: 1
                horizontalAlignment: Text.AlignHCenter
                size: 26
                color: simplifiedUI.colors.text.black
            }

            MouseArea {
                id: raiseHandButtonMouseArea
                anchors.fill: parent
                hoverEnabled: enabledTabletEnums.ButtonClick);
                    sendToScript({

                onClicked: {
                    Tablet.playSound(TabletEnums.ButtonClick);
                    sendToScript({
                        "source": "EmoteAppBar.qml",
                        "method": "raiseHandPressed"
                    });
                    parent.color = simplifiedUI.colors.darkBackground
                }

                onEntered: {
                    console.log("ENTERED BUTTON");
                    Tablet.playSound(TabletEnums.ButtonHover);
                    parent.color = simplifiedUI.colors.darkBackground
                }

                onExited: {
                    parent.color = simplifiedUI.colors.white
                }
            }
        }

        Rectangle {
            id: applaudButtonContainer
            width: root.originalWidth
            height: drawerContainer.height
            color: simplifiedUI.colors.white

            HifiStylesUit.GraphikRegular {
                text: "B"
                anchors.fill: parent
                anchors.rightMargin: 1
                horizontalAlignment: Text.AlignHCenter
                size: 26
                color: simplifiedUI.colors.text.black
            }

            MouseArea {
                id: applaudButtonMouseArea
                anchors.fill: parent
                hoverEnabled: enab
                        "source": "EmoteAppBarled

                onClicked: {
                    Tablet.playSound(TabletEnums.ButtonClick);
                    sendToScript({
                        "source": "EmoteAppBar.qml",
                        "method": "applaudPressed"
                    });
                    parent.color = simplifiedUI.colors.darkBackground
                }

                onEntered: {
                    console.log("ENTERED BUTTON");
                    Tablet.playSound(TabletEnums.ButtonHover);
                    parent.color = simplifiedUI.colors.darkBackground
                }

                onExited: {
                    parent.color = simplifiedUI.colors.white
                }
            }
        }

        Rectangle {
            id: pointButtonContainer
            width: root.originalWidth
            height: drawerContainer.height
            color: simplifiedUI.colors.white

            HifiStylesUit.GraphikRegular {
                text: "N"
                anchors.fill: parent
                anchors.rightMargin: 1
                horizontalAlignment: Text.AlignHCenter
                size: 26
                color: simplifiedUI.colors.text.black
            }

            MouseArea {
                id: pointButtonMouseArea
                anchors.fill: parent
                hoverEnabled: enabledoteAppBar.qml",
                        "method": "pointPresse

                onClicked: {
                    Tablet.playSound(TabletEnums.ButtonClick);
                    sendToScript({
                        "source": "EmoteAppBar.qml",
                        "method": "pointPressed"
                    });
                    parent.color = simplifiedUI.colors.darkBackground
                }

                onEntered: {
                    console.log("ENTERED BUTTON");
                    Tablet.playSound(TabletEnums.ButtonHover);
                    parent.color = simplifiedUI.colors.darkBackground
                }

                onExited: {
                    parent.color = simplifiedUI.colors.white
                }
            }
        }

        Rectangle {
            id: emojiButtonContainer
            width: root.originalWidth
            height: drawerContainer.height
            color: simplifiedUI.colors.white

            HifiStylesUit.GraphikRegular {
                text: "😊"
                anchors.fill: parent
                anchors.rightMargin: 1
                horizontalAlignment: Text.AlignHCenter
                size: 26
                color: simplifiedUI.colors.text.black
            }

            MouseArea {
                id: emojiButtonMouseArea
                anchors.fill: parent
                hoverEnabled: enabledggleEmojiApp"
                    });

                onClicked: {
                    Tablet.playSound(TabletEnums.ButtonClick);
                    sendToScript({
                        "source": "EmoteAppBar.qml",
                        "method": "toggleEmojiApp"
                    });
                    parent.color = simplifiedUI.colors.darkBackground
                }

                onEntered: {
                    console.log("ENTERED BUTTON");
                    Tablet.playSound(TabletEnums.ButtonHover);
                    parent.color = simplifiedUI.colors.darkBackground
                }

                onExited: {
                    parent.color = simplifiedUI.colors.white
                }
            }
        }
    }

    signal sendToScript(var message);
    signal requestNewWidth(int newWidth);
}
