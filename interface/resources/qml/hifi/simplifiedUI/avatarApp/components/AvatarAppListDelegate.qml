//
//  AvatarAppListDelegate.qml
//
//  Created by Zach Fox on 2019-05-09
//  Copyright 2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import QtQuick 2.10
import "../../simplifiedConstants" as SimplifiedConstants
import "../../simplifiedControls" as SimplifiedControls
import stylesUit 1.0 as HifiStylesUit
import QtGraphicalEffects 1.0

Rectangle {
    id: root
    
    SimplifiedConstants.SimplifiedConstants {
        id: simplifiedUI
    }

    property string itemName
    property string itemPreviewImageUrl
    property string itemHref
    property bool standaloneOptimized
    property bool standaloneIncompatible
    property bool isCurrentItem

    property bool isHovering: mouseArea.containsMouse || wearButton.hovered || wearButton.down

    height: 102
    width: parent.width
    color: root.isHovering ? simplifiedUI.colors.darkBackgroundHighlight : "transparent"
    

    Rectangle {
        id: borderMask
        width: root.isHovering ? itemPreviewImage.width + 4 : itemPreviewImage.width - 4
        height: width
        radius: width
        anchors.centerIn: itemPreviewImage
        color: "#FFFFFF"

        Behavior on width {
            enabled: true
            SmoothedAnimation { velocity: 80 }
        }
    }

    Image {
        id: itemPreviewImage
        source: root.itemPreviewImageUrl
        anchors.left: parent.left
        anchors.leftMargin: 20
        anchors.verticalCenter: parent.verticalCenter
        height: 60
        width: height
        fillMode: Image.PreserveAspectCrop
        mipmap: true
        layer.enabled: true
        layer.effect: OpacityMask {
            maskSource: mask
        }

        Rectangle {
            id: mask
            width: itemPreviewImage.width
            height: itemPreviewImage.height
            radius: itemPreviewImage.width / 2
            visible: false
        }
    }

    HifiStylesUit.GraphikRegular {
        id: avatarName
        text: root.itemName
        anchors.left: itemPreviewImage.right
        anchors.leftMargin: 20
        anchors.right: root.isHovering ? wearButton.left : parent.right
        anchors.rightMargin: 20
        anchors.verticalCenter: parent.verticalCenter
        elide: Text.ElideRight
        height: parent.height
        size: 20
        color: simplifiedUI.colors.text.almostWhite
    }

    SimplifiedControls.Button {
        id: wearButton
        visible: MyAvatar.skeletonModelURL !== root.itemHref && root.isHovering

        anchors.right: parent.right
        anchors.rightMargin: 24
        anchors.verticalCenter: parent.verticalCenter
        width: 165
        height: 32
        text: "WEAR"

        onClicked: {
            MyAvatar.useFullAvatarURL(root.itemHref);
        }
    }

    SimplifiedControls.CheckBox {
        id: wornCheckBox
        enabled: false
        visible: MyAvatar.skeletonModelURL === root.itemHref
        anchors.right: parent.right
        anchors.rightMargin: 24
        anchors.verticalCenter: parent.verticalCenter
        width: 14
        height: 14
        checked: true
    }

    MouseArea {
        z: -1
        id: mouseArea
        anchors.fill: parent
        hoverEnabled: true
    }
}
