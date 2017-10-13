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
import QtQuick.Controls 1.4 as Original
import QtQuick.Controls.Styles 1.4

import "../styles-uit"
import "../controls-uit" as HifiControls

Original.RadioButton {
    id: radioButton
    HifiConstants { id: hifi }

    property int colorScheme: hifi.colorSchemes.light
    readonly property bool isLightColorScheme: colorScheme == hifi.colorSchemes.light

    readonly property int boxSize: 14
    readonly property int boxRadius: 3
    readonly property int checkSize: 10
    readonly property int checkRadius: 2

    style: RadioButtonStyle {
        indicator: Rectangle {
            id: box
            width: boxSize
            height: boxSize
            radius: 7
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
                radius: 7
                anchors.centerIn: parent
                color: "#00B4EF"
                border.width: 1
                border.color: "#36CDFF"
                visible: checked && !pressed || !checked && pressed
            }
        }

        label: RalewaySemiBold {
            text: control.text
            size: hifi.fontSizes.inputLabel
            color: isLightColorScheme ? hifi.colors.lightGray : hifi.colors.lightGrayText
            x: radioButton.boxSize / 2
        }
    }
}
