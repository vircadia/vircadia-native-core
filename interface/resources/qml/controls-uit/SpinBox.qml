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
    property string labelInside: ""
    property color colorLabelInside: hifi.colors.white
    property real controlHeight: height + (spinBoxLabel.visible ? spinBoxLabel.height + spinBoxLabel.anchors.bottomMargin : 0)

    FontLoader { id: firaSansSemiBold; source: "../../fonts/FiraSans-SemiBold.ttf"; }
    font.family: firaSansSemiBold.name
    font.pixelSize: hifi.fontSizes.textFieldInput
    height: hifi.fontSizes.textFieldInput + 13  // Match height of TextField control.

    y: spinBoxLabel.visible ? spinBoxLabel.height + spinBoxLabel.anchors.bottomMargin : 0

    style: SpinBoxStyle {
        id: spinStyle
        background: Rectangle {
            color: isLightColorScheme
                   ? (spinBox.activeFocus ? hifi.colors.white : hifi.colors.lightGray)
                   : (spinBox.activeFocus ? hifi.colors.black : hifi.colors.baseGrayShadow)
            border.color: spinBoxLabelInside.visible ? spinBoxLabelInside.color : hifi.colors.primaryHighlight
            border.width: spinBox.activeFocus ? spinBoxLabelInside.visible ? 2 : 1 : 0
        }

        textColor: isLightColorScheme
                   ? (spinBox.activeFocus ? hifi.colors.black : hifi.colors.lightGray)
                   : (spinBox.activeFocus ? hifi.colors.white : hifi.colors.lightGrayText)
        selectedTextColor: hifi.colors.black
        selectionColor: hifi.colors.primaryHighlight

        horizontalAlignment: Qt.AlignLeft
        padding.left: spinBoxLabelInside.visible ? 30 : hifi.dimensions.textPadding
        padding.right: hifi.dimensions.spinnerSize
        padding.top: 0

        incrementControl: HiFiGlyphs {
            id: incrementButton
            text: hifi.glyphs.caratUp
            x: 10
            y: 1
            size: hifi.dimensions.spinnerSize
            color: styleData.upPressed ? (isLightColorScheme ? hifi.colors.black : hifi.colors.white) : hifi.colors.gray
        }

        decrementControl: HiFiGlyphs {
            text: hifi.glyphs.caratDn
            x: 10
            y: -1
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

    HifiControls.Label {
        id: spinBoxLabelInside
        text: spinBox.labelInside
        anchors.left: parent.left
        anchors.leftMargin: 10
        font.bold: true
        anchors.verticalCenter: parent.verticalCenter
        color: spinBox.colorLabelInside
        visible: spinBox.labelInside != ""
    }

    MouseArea {
        anchors.fill: parent
        propagateComposedEvents: true
        onWheel: {
            if(spinBox.activeFocus)
                wheel.accepted = false
            else
                wheel.accepted = true
        }
        onPressed: {
            mouse.accepted = false
        }
        onReleased: {
            mouse.accepted = false
        }
        onClicked: {
            mouse.accepted = false
        }
        onDoubleClicked: {
            mouse.accepted = false
        }
    }
}
