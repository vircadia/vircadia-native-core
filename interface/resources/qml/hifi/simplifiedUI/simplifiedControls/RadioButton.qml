//
//  RadioButton.qml
//
//  Created by Zach Fox on 2019-05-06
//  Copyright 2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import QtQuick 2.10
import QtQuick.Controls 2.3
import TabletScriptingInterface 1.0
import "../simplifiedConstants" as SimplifiedConstants

RadioButton {
    id: root

    SimplifiedConstants.SimplifiedConstants {
        id: simplifiedUI
    }

    padding: 0
    property alias labelTextColor: radioButtonLabel.color
    property alias labelFontSize: radioButtonLabel.font.pixelSize
    property int radioButtonRadius: 14
    property int labelLeftMargin: simplifiedUI.margins.controls.radioButton.labelLeftMargin
    property bool wrapLabel: true
    readonly property int checkSize: Math.max(root.radioButtonRadius - 8, 10)
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
        id: radioButtonIndicator
        implicitWidth: root.radioButtonRadius
        implicitHeight: root.radioButtonRadius
        radius: root.radioButtonRadius
        y: parent.height / 2 - height / 2
        border.width: root.hovered ? simplifiedUI.sizes.controls.radioButton.outerBorderWidth : 0
        border.color: simplifiedUI.colors.controls.radioButton.hover.outerBorderColor
        opacity: root.disabled ? 0.5 : 1.0

        gradient: Gradient {
            GradientStop {
                position: simplifiedUI.colors.controls.radioButton.background.startPosition
                color: simplifiedUI.colors.controls.radioButton.background.startColor
            }
            GradientStop {
                position: simplifiedUI.colors.controls.radioButton.background.endPosition
                color: simplifiedUI.colors.controls.radioButton.background.endColor
            }
        }

        Rectangle {
            id: innerBox
            visible: root.checked || root.hovered
            anchors.centerIn: parent
            width: root.checkSize
            height: width
            radius: checkSize / 2
            border.width: simplifiedUI.sizes.controls.radioButton.innerBorderWidth
            border.color: root.hovered ? simplifiedUI.colors.controls.radioButton.hover.innerBorderColor : simplifiedUI.colors.controls.radioButton.checked.innerBorderColor
            color: root.hovered ? simplifiedUI.colors.controls.radioButton.hover.innerColor : simplifiedUI.colors.controls.radioButton.hover.innerColor
            opacity: root.hovered ? simplifiedUI.colors.controls.radioButton.hover.innerOpacity : 1.0
        }

        Rectangle {
            id: pressedBox
            visible: root.pressed
            width: parent.width
            height: parent.height
            radius: parent.radius
            anchors.centerIn: parent
            color: simplifiedUI.colors.controls.radioButton.active.color
        }
    }

    contentItem: Text {
        id: radioButtonLabel
        font.pixelSize: 14
        font.family: "Graphik"
        font.weight: Font.Normal
        text: root.text
        color: simplifiedUI.colors.text.white
        x: 2
        wrapMode: root.wrapLabel ? Text.Wrap : Text.NoWrap
        elide: root.wrapLabel ? Text.ElideNone : Text.ElideRight
        enabled: root.enabled
        verticalAlignment: Text.AlignVCenter
        leftPadding: radioButtonIndicator.width + root.labelLeftMargin
        topPadding: -3 // For perfect alignment when using Graphik
    }
}
