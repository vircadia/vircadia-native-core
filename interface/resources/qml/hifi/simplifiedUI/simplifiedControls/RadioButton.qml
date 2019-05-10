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
    property bool wrapLabel: true;
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
        border.width: 1
        border.color: pressed || hovered
                      ? hifi.colors.checkboxCheckedBorder
                      : hifi.colors.checkboxDarkFinish

        gradient: Gradient {
            GradientStop {
                position: 0.2
                color: pressed || hovered
                       ? hifi.colors.checkboxLightStart
                       : hifi.colors.checkboxDarkStart
            }
            GradientStop {
                position: 1.0
                color: pressed || hovered
                       ? hifi.colors.checkboxLightFinish
                       : hifi.colors.checkboxDarkFinish
            }
        }

        Rectangle {
            id: innerBox
            visible: pressed || hovered
            anchors.centerIn: parent
            width: checkSize - 4
            height: width
            radius: checkSize / 2
            color: hifi.colors.checkboxCheckedBorder
        }

        Rectangle {
            id: check
            width: checkSize
            height: checkSize
            radius: checkSize / 2
            anchors.centerIn: parent
            color: hifi.colors.checkboxChecked
            border.width: 2
            border.color: hifi.colors.checkboxCheckedBorder
            visible: checked && !pressed || !checked && pressed
        }

        Rectangle {
            id: disabledOverlay
            visible: !enabled
            width: root.radioButtonRadius
            height: root.radioButtonRadius
            radius: root.radioButtonRadius / 2
            border.width: 1
            border.color: hifi.colors.baseGrayHighlight
            color: hifi.colors.baseGrayHighlight
            opacity: 0.5
        }
    }

    contentItem: Text {
        id: radioButtonLabel
        height: root.radioButtonRadius
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
    }
}
