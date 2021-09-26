//
//  SpinBox.qml
//
//  Created by David Rowe on 26 Feb 2016
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import QtQuick 2.7
import QtQuick.Controls 2.2

import "../stylesUit"
import "." as HifiControls

SpinBox {
    id: spinBox

    HifiConstants {
        id: hifi
    }

    inputMethodHints: Qt.ImhFormattedNumbersOnly
    property int colorScheme: hifi.colorSchemes.light
    readonly property bool isLightColorScheme: colorScheme === hifi.colorSchemes.light
    property string label: ""
    property string suffix: ""
    property string labelInside: ""
    property color colorLabelInside: hifi.colors.white
    property color backgroundColor: isLightColorScheme
                                    ? (spinBox.activeFocus ? hifi.colors.white : hifi.colors.lightGray)
                                    : (spinBox.activeFocus ? hifi.colors.black : hifi.colors.baseGrayShadow)
    property real controlHeight: height + (spinBoxLabel.visible ? spinBoxLabel.height + spinBoxLabel.anchors.bottomMargin : 0)
    property int decimals: 2;
    property real factor: Math.pow(10, decimals)

    property real minimumValue: 0.0
    property real maximumValue: 0.0

    property real realValue: 0.0
    property real realFrom: minimumValue
    property real realTo: maximumValue
    property real realStepSize: 1.0

    signal editingFinished()

    implicitHeight: height
    implicitWidth: width
    editable: true

    padding: 0
    leftPadding: 0
    rightPadding: padding + (up.indicator ? up.indicator.width : 0)
    topPadding: 0
    bottomPadding: 0

    locale: Qt.locale("en_US")

    onValueModified: {
        realValue = value / factor
    }

    onValueChanged: {
        realValue = value / factor
        spinBox.editingFinished();
    }

    onRealValueChanged: {
        var newValue = Math.round(realValue * factor);
        if(value != newValue) {
            value = newValue;
        }
    }

    stepSize: Math.round(realStepSize * factor)
    to : Math.round(realTo*factor)
    from : Math.round(realFrom*factor)

    font.family: "Fira Sans SemiBold"
    font.pixelSize: hifi.fontSizes.textFieldInput
    height: hifi.fontSizes.textFieldInput + 13  // Match height of TextField control.

    y: spinBoxLabel.visible ? spinBoxLabel.height + spinBoxLabel.anchors.bottomMargin : 0

    background: Rectangle {
        color: backgroundColor
        border.color: spinBoxLabelInside.visible ? spinBoxLabelInside.color : hifi.colors.primaryHighlight
        border.width: spinBox.activeFocus ? spinBoxLabelInside.visible ? 2 : 1 : 0
    }

    validator: DoubleValidator {
        decimals: spinBox.decimals
        bottom: Math.min(spinBox.realFrom, spinBox.realTo)
        top:  Math.max(spinBox.realFrom, spinBox.realTo)
        notation: DoubleValidator.StandardNotation
    }

    textFromValue: function(value, locale) {
        return (value / factor).toFixed(decimals);
    }

    valueFromText: function(text, locale) {
        return Math.round(Number.fromLocaleString(locale, text) * factor);
    }


    contentItem: TextInput {
        id: spinboxText
        z: 2
        color: isLightColorScheme
               ? (spinBox.activeFocus ? hifi.colors.black : hifi.colors.baseGrayHighlight)
               : (spinBox.activeFocus ? hifi.colors.white : hifi.colors.lightGrayText)
        selectedTextColor: hifi.colors.black
        selectionColor: hifi.colors.primaryHighlight
        text: spinBox.textFromValue(spinBox.value, spinBox.locale)
        inputMethodHints: spinBox.inputMethodHints
        validator: spinBox.validator
        verticalAlignment: Qt.AlignVCenter
        leftPadding: spinBoxLabelInside.visible ? 30 : hifi.dimensions.textPadding
        width: spinBox.width - hifi.dimensions.spinnerSize
        Text {
            id: suffixText
            x: metrics.advanceWidth(spinboxText.text + '*')
            height: spinboxText.height

            FontMetrics {
                id: metrics
                font: spinboxText.font
            }

            color: isLightColorScheme
                   ? (spinBox.activeFocus ? hifi.colors.black : hifi.colors.baseGrayHighlight)
                   : (spinBox.activeFocus ? hifi.colors.white : hifi.colors.lightGrayText)
            text: suffix
            verticalAlignment: Qt.AlignVCenter
        }
    }

    up.indicator: Item {
        x: spinBox.width - implicitWidth - 5
        y: 1
        clip: true
        implicitHeight: spinBox.implicitHeight/2
        implicitWidth: spinBox.implicitHeight/2
        HiFiGlyphs {
            anchors.centerIn: parent
            text: hifi.glyphs.caratUp
            size: hifi.dimensions.spinnerSize
            color: spinBox.up.pressed || spinBox.up.hovered ? (isLightColorScheme ? hifi.colors.black : hifi.colors.white) : hifi.colors.gray
        }
    }
    up.onPressedChanged: {
        if(value) {
            spinBox.forceActiveFocus();
        }
    }

    down.indicator: Item {
            x: spinBox.width - implicitWidth - 5
            y: spinBox.implicitHeight/2
            clip: true
            implicitHeight: spinBox.implicitHeight/2
            implicitWidth: spinBox.implicitHeight/2
            HiFiGlyphs {
                anchors.centerIn: parent
                text: hifi.glyphs.caratDn
                size: hifi.dimensions.spinnerSize
                color: spinBox.down.pressed || spinBox.down.hovered ? (isLightColorScheme ? hifi.colors.black : hifi.colors.white) : hifi.colors.gray
            }
    }
    down.onPressedChanged: {
        if(value) {
            spinBox.forceActiveFocus();
        }
    }

    Keys.onPressed: {
        if (event.key === Qt.Key_Return) {
            if (!spinboxText.acceptableInput) {
                var number = spinBox.valueFromText(spinboxText.text, spinBox.locale) / spinBox.factor

                if (number < spinBox.minimumValue) {
                    number = spinBox.minimumValue;
                } else if (number > maximumValue) {
                    number = spinBox.maximumValue;
                }

                spinboxText.text = spinBox.textFromValue(Math.round(number * factor), spinBox.locale)
            }
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
        acceptedButtons: Qt.NoButton
        onWheel: {
            if (wheel.angleDelta.y > 0)
                value += stepSize
            else
                value -= stepSize
        }
    }
}
