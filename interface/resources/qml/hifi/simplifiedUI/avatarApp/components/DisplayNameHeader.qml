//
//  DisplayNameHeader.qml
//
//  Created by Wayne Chen on 2019-05-03
//  Copyright 2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import QtQuick 2.10
import "../../simplifiedConstants" as SimplifiedConstants
import stylesUit 1.0 as HifiStylesUit
import controlsUit 1.0 as HifiControlsUit
import QtGraphicalEffects 1.0

Item {
    id: root

    SimplifiedConstants.SimplifiedConstants {
        id: simplifiedUI
    }

    height: itemPreviewImage.height
    property string previewUrl: ""
    property bool loading: true

    AnimatedImage {
        visible: root.loading
        anchors.left: parent.left
        anchors.verticalCenter: parent.verticalCenter
        height: 72
        width: height
        source: "../../images/loading.gif"
    }

    Image {
        id: itemPreviewImage
        visible: !root.loading
        source: root.previewUrl
        anchors.left: parent.left
        anchors.verticalCenter: parent.verticalCenter
        height: 100
        width: height
        fillMode: Image.PreserveAspectCrop
        layer.enabled: true
        layer.effect: OpacityMask {
            maskSource: mask
        }
        mipmap: true

        Rectangle {
            id: mask
            width: itemPreviewImage.width
            height: width
            radius: width
            visible: false
        }
    }

    Item {
        id: displayNameContainer
        height: itemPreviewImage.height
        anchors.right: parent.right
        anchors.left: itemPreviewImage.right
        anchors.leftMargin: 21
        anchors.verticalCenter: parent.verticalCenter

        HifiStylesUit.GraphikRegular {
            id: displayNameLabel
            text: "Display Name"
            color: simplifiedUI.colors.text.lightGrey
            size: 16
            anchors.top: parent.top
            anchors.right: parent.right
            anchors.bottom: parent.verticalCenter
            anchors.left: parent.left
            verticalAlignment: Text.AlignBottom
        }

        Item {
            id: myDisplayNameContainer
            // Size
            width: parent.width
            height: 40
            anchors.top: parent.verticalCenter
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            anchors.left: parent.left
            
            MouseArea {
                anchors.fill: parent
                hoverEnabled: true
                onClicked: {
                    myDisplayNameText.focus = true;
                    myDisplayNameText.cursorPosition = myDisplayNameText.positionAt(mouseX - myDisplayNameText.anchors.leftMargin, mouseY, TextInput.CursorOnCharacter);
                }
                onDoubleClicked: {
                    myDisplayNameText.selectAll();
                    myDisplayNameText.focus = true;
                }
            }

            TextInput {
                id: myDisplayNameText
                text: MyAvatar.sessionDisplayName === "" ? MyAvatar.displayName : MyAvatar.sessionDisplayName
                maximumLength: 256
                clip: true
                anchors.fill: parent
                color: simplifiedUI.colors.text.white
                font.family: "Graphik Medium"
                font.pixelSize: 22
                selectionColor: simplifiedUI.colors.text.white
                selectedTextColor: simplifiedUI.colors.text.darkGrey
                verticalAlignment: TextInput.AlignVCenter
                horizontalAlignment: TextInput.AlignLeft
                autoScroll: false
                onEditingFinished: {
                    if (MyAvatar.displayName !== text) {
                        MyAvatar.displayName = text;
                    }
                    myDisplayNameText.focus = false;
                }
                onFocusChanged: {
                    if (!focus) {
                        cursorPosition = 0;
                    }
                    myDisplayNameText.autoScroll = focus;
                }
            }
        }
    }
}
