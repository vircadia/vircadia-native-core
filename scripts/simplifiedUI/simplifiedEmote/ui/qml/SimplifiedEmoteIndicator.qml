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
    color: simplifiedUI.colors.almostWhite
    anchors.fill: parent

    property int originalWidth: 48
    property int hoveredWidth: 357
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
            emoteButtonsRepeater.ItemAt(buttonsModel.count -1).buttonMouseArea.hoverEnabled = true;
        }
        onExited: {
            Tablet.playSound(TabletEnums.ButtonClick);
            root.requestedWidth = root.originalWidth;
        }
        z: 2
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

    /*
        MILAD NOTE:
        The main container of the actual buttons
    */

    Rectangle {
        id: drawerContainer
        z: 1
        color: simplifiedUI.colors.almostWhite
        anchors.top: parent.top
        anchors.right: parent.right
        height: parent.height
        width: root.hoveredWidth

        /*
            MILAD NOTE:
            Below is one button.  They are just text representations now, but
            they probably be switched with Image {} to be supplied by Joshua.
        */

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
                z: 3
                width: root.originalWidth
                height: parent.height
                color: simplifiedUI.colors.darkBackground

                HifiStylesUit.GraphikRegular {
                    text: text
                    z: 3
                    anchors.fill: parent
                    anchors.rightMargin: 1
                    horizontalAlignment: Text.AlignHCenter
                    size: 26
                    color: simplifiedUI.colors.text.black
                }

                MouseArea {
                    id: buttonMouseArea
                    anchors.fill: parent
                    hoverEnabled: false
                    propagateComposedEvents: true;
                    z: 4
                    onClicked: {
                        sendToScript({
                            "source": "EmoteAppBar.qml",
                            "method": method
                        });
                    }
                    onEntered: {
                    }
                }
            }
        }
    }

    signal sendToScript(var message);
    signal requestNewWidth(int newWidth);
}
