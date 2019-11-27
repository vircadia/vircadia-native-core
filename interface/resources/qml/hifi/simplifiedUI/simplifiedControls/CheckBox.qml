//
//  CheckBox.qml
//
//  Created by Zach Fox on 2019-05-14
//  Copyright 2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import QtQuick 2.10
import QtQuick.Controls 2.3 as Original
import stylesUit 1.0 as HifiStylesUit
import "../simplifiedConstants" as SimplifiedConstants
import TabletScriptingInterface 1.0

Original.CheckBox {
    id: root

    SimplifiedConstants.SimplifiedConstants {
        id: simplifiedUI
    }

    property int colorScheme: hifi.colorSchemes.light
    property string color: hifi.colors.lightGrayText
    property alias checkBoxSize: checkBoxIndicator.width
    property alias checkBoxRadius: checkBoxIndicator.radius
    property alias checkSize: innerBox.width
    property alias checkRadius: innerBox.radius
    property bool wrapLabel: true
    property alias labelFontFamily: checkBoxLabel.font.family
    property alias labelFontSize: checkBoxLabel.font.pixelSize
    property alias labelFontWeight: checkBoxLabel.font.weight

    focusPolicy: Qt.ClickFocus
    hoverEnabled: true

    onClicked: {
        Tablet.playSound(TabletEnums.ButtonClick);
    }


    onHoveredChanged: {
        if (hovered) {
            Tablet.playSound(TabletEnums.ButtonHover);
        }
    }


    indicator: Rectangle {
        id: checkBoxIndicator
        width: 14
        height: width
        radius: 4
        y: parent.height / 2 - height / 2
        color: root.enabled ?
            (root.pressed ? simplifiedUI.colors.controls.checkBox.background.active : simplifiedUI.colors.controls.checkBox.background.enabled) :
            simplifiedUI.colors.controls.checkBox.background.disabled
        border.width: root.hovered ? 2 : 0
        border.color: simplifiedUI.colors.controls.checkBox.border.hover

        Rectangle {
            id: innerBox
            visible: root.hovered || root.checked
            opacity: root.hovered ? 0.3 : 1.0
            anchors.centerIn: parent
            width: checkBoxIndicator.width - 4
            height: width
            radius: 2
            color: simplifiedUI.colors.controls.checkBox.innerBox.background
            border.width: 1
            border.color: simplifiedUI.colors.controls.checkBox.innerBox.border
        }
    }

    contentItem: Text {
        id: checkBoxLabel
        text: root.text
        color: root.color
        font.family: "Graphik"
        font.pixelSize: 14
        font.weight: Font.DemiBold
        x: 2
        verticalAlignment: Text.AlignVCenter
        wrapMode: root.wrapLabel ? Text.Wrap : Text.NoWrap
        elide: root.wrapLabel ? Text.ElideNone : Text.ElideRight
        leftPadding: root.indicator.width + root.spacing
    }
}
