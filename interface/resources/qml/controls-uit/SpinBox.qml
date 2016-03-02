//
//  SpinBox.qml
//
//  Created by David Rowe on 26 Feb 2016
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import QtQuick 2.5
import QtQuick.Controls 1.4
import QtQuick.Controls.Styles 1.4

import "../styles-uit"
import "../controls-uit" as HifiControls

SpinBox {
    id: spinBox

    property int colorScheme: hifi.colorSchemes.light
    readonly property bool isLightColorScheme: colorScheme == hifi.colorSchemes.light
    property string label: ""
    property real controlHeight: height + (spinBoxLabel.visible ? spinBoxLabel.height + spinBoxLabel.anchors.bottomMargin : 0)

    FontLoader { id: firaSansSemiBold; source: "../../fonts/FiraSans-SemiBold.ttf"; }
    font.family: firaSansSemiBold.name
    font.pixelSize: hifi.fontSizes.textFieldInput
    height: hifi.fontSizes.textFieldInput + 14  // Match height of TextField control.

    y: spinBoxLabel.visible ? spinBoxLabel.height + spinBoxLabel.anchors.bottomMargin : 0

    style: SpinBoxStyle {
        background: Rectangle {
            color: isLightColorScheme
                   ? (spinBox.focus ? hifi.colors.white : hifi.colors.lightGray)
                   : (spinBox.focus ? hifi.colors.black : hifi.colors.baseGrayShadow)
            border.color: hifi.colors.primaryHighlight
            border.width: spinBox.focus ? 1 : 0
        }

        textColor: isLightColorScheme
                   ? (spinBox.focus ? hifi.colors.black : hifi.colors.lightGray)
                   : (spinBox.focus ? hifi.colors.white : hifi.colors.lightGrayText)
        selectedTextColor: hifi.colors.black
        selectionColor: hifi.colors.primaryHighlight

        horizontalAlignment: Qt.AlignLeft
        padding.left: hifi.dimensions.textPadding
        padding.right: hifi.dimensions.spinnerSize

        incrementControl: HiFiGlyphs {
            id: incrementButton
            text: hifi.glyphs.caratUp
            x: 6
            y: 2
            size: hifi.dimensions.spinnerSize
            color: styleData.upPressed ? (isLightColorScheme ? hifi.colors.black : hifi.colors.white) : hifi.colors.gray
        }

        decrementControl: HiFiGlyphs {
            text: hifi.glyphs.caratDn
            x: 6
            y: -3
            size: hifi.dimensions.spinnerSize
            color: styleData.downPressed ? (isLightColorScheme ? hifi.colors.black : hifi.colors.white) : hifi.colors.gray
        }
    }

    HifiControls.Label {
        id: spinBoxLabel
        text: spinBox.label
        colorScheme: spinBox.colorScheme
        anchors.left: parent.left
        anchors.bottom: parent.top
        anchors.bottomMargin: 4
        visible: label != ""
    }
}
