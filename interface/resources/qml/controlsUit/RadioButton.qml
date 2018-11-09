//
//  RadioButton.qml
//
//  Created by Cain Kilgore on 20th July 2017
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import QtQuick 2.5
import QtQuick.Controls 2.2 as Original

import "../stylesUit"
import "." as HifiControls

import TabletScriptingInterface 1.0

Original.RadioButton {
    id: radioButton
    HifiConstants { id: hifi }

    hoverEnabled: true

    property int colorScheme: hifi.colorSchemes.light
    readonly property bool isLightColorScheme: colorScheme == hifi.colorSchemes.light

    property real letterSpacing: 1
    property int fontSize: hifi.fontSizes.inputLabel
    property int boxSize: defaultBoxSize
    property real scaleFactor: boxSize / defaultBoxSize

    readonly property int defaultBoxSize: 14
    readonly property int boxRadius: 3 * scaleFactor
    readonly property int checkSize: 10 * scaleFactor
    readonly property int checkRadius: 2 * scaleFactor
    readonly property int indicatorRadius: 7 * scaleFactor

    onClicked: {
        Tablet.playSound(TabletEnums.ButtonClick);
    }

    onHoveredChanged: {
        if (hovered) {
            Tablet.playSound(TabletEnums.ButtonHover);
        }
    }

    indicator: Rectangle {
        id: box
        width: boxSize
        height: boxSize
        radius: indicatorRadius
        x: radioButton.leftPadding
        y: parent.height / 2 - height / 2
        gradient: Gradient {
            GradientStop {
                position: 0.2
                color: pressed || hovered
                       ? (radioButton.isLightColorScheme ? hifi.colors.checkboxDarkStart : hifi.colors.checkboxLightStart)
                       : (radioButton.isLightColorScheme ? hifi.colors.checkboxLightStart : hifi.colors.checkboxDarkStart)
            }
            GradientStop {
                position: 1.0
                color: pressed || hovered
                       ? (radioButton.isLightColorScheme ? hifi.colors.checkboxDarkFinish : hifi.colors.checkboxLightFinish)
                       : (radioButton.isLightColorScheme ? hifi.colors.checkboxLightFinish : hifi.colors.checkboxDarkFinish)
            }
        }

        Rectangle {
            id: check
            width: checkSize
            height: checkSize
            radius: indicatorRadius
            anchors.centerIn: parent
            color: "#00B4EF"
            border.width: 1
            border.color: "#36CDFF"
            visible: checked && !pressed || !checked && pressed
        }
    }

    contentItem: RalewaySemiBold {
        text: radioButton.text
        size: radioButton.fontSize
        font.letterSpacing: letterSpacing
        color: isLightColorScheme ? hifi.colors.lightGray : hifi.colors.lightGrayText
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        leftPadding: radioButton.indicator.width + radioButton.spacing
    }
}
