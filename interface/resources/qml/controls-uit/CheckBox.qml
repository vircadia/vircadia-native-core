//
//  CheckBox.qml
//
//  Created by David Rowe on 26 Feb 2016
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import QtQuick 2.5
import QtQuick.Controls 1.4 as Original
import QtQuick.Controls.Styles 1.4

import "../styles-uit"

Original.CheckBox {
    id: checkBox
    HifiConstants { id: hifi }

    property int colorScheme: hifi.colorSchemes.light
    readonly property bool isLightColorScheme: colorScheme == hifi.colorSchemes.light

    property int boxSize: 14
    readonly property int boxRadius: 3
    readonly property int checkSize: 10
    readonly property int checkRadius: 2

    style: CheckBoxStyle {
        indicator: Rectangle {
            id: box
            width: boxSize
            height: boxSize
            radius: boxRadius
            gradient: Gradient {
                GradientStop {
                    position: 0.2
                    color: pressed || hovered
                           ? (checkBox.isLightColorScheme ? hifi.colors.checkboxDarkStart : hifi.colors.checkboxLightStart)
                           : (checkBox.isLightColorScheme ? hifi.colors.checkboxLightStart : hifi.colors.checkboxDarkStart)
                }
                GradientStop {
                    position: 1.0
                    color: pressed || hovered
                           ? (checkBox.isLightColorScheme ? hifi.colors.checkboxDarkFinish : hifi.colors.checkboxLightFinish)
                           : (checkBox.isLightColorScheme ? hifi.colors.checkboxLightFinish : hifi.colors.checkboxDarkFinish)
                }
            }

            Rectangle {
                id: check
                width: checkSize
                height: checkSize
                radius: checkRadius
                anchors.centerIn: parent
                color: hifi.colors.checkboxChecked
                border.width: 1
                border.color: hifi.colors.checkboxCheckedBorder
                visible: checked && !pressed || !checked && pressed
            }
        }

        label: Label {
            text: control.text
            colorScheme: checkBox.colorScheme
            x: checkBox.boxSize / 2
            wrapMode: Text.Wrap
            enabled: checkBox.enabled
        }
    }
}
