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
import "../../simplifiedControls" as SimplifiedControls
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
        sourceSize.width: width
        sourceSize.height: height
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
            radius: width / 2
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
            width: parent.width
            height: 42
            anchors.top: parent.verticalCenter
            anchors.right: parent.right
            anchors.left: parent.left

            SimplifiedControls.TextField {
                id: myDisplayNameText
                rightGlyph: simplifiedUI.glyphs.pencil
                text: MyAvatar.displayName
                maximumLength: 256
                clip: true
                selectByMouse: true
                anchors.fill: parent
                onEditingFinished: {
                    if (MyAvatar.displayName !== text) {
                        MyAvatar.displayName = text;
                    }
                    myDisplayNameText.focus = false;
                }
                onFocusChanged: {
                    myDisplayNameText.autoScroll = focus;
                }
            }
        }
    }
}
