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

import TabletScriptingInterface 1.0

Original.CheckBox {
    id: checkBox

    property int colorScheme: hifi.colorSchemes.light
    property string color: hifi.colors.lightGrayText
    readonly property bool isLightColorScheme: colorScheme == hifi.colorSchemes.light
    property bool isRedCheck: false
    property int boxSize: 14
    property int boxRadius: 3
    property bool wrap: true;
    readonly property int checkSize: Math.max(boxSize - 8, 10)
    readonly property int checkRadius: 2
    activeFocusOnPress: true

    onClicked: {
        Tablet.playSound(TabletEnums.ButtonClick);
    }

// TODO: doesnt works for QQC1. check with QQC2
//    onHovered: {
//        Tablet.playSound(TabletEnums.ButtonHover);
//    }

    style: CheckBoxStyle {
        indicator: Rectangle {
            id: box
            width: boxSize
            height: boxSize
            radius: boxRadius
            border.width: 1
            border.color: pressed || hovered
                          ? hifi.colors.checkboxCheckedBorder
                          : (checkBox.isLightColorScheme ? hifi.colors.checkboxLightFinish : hifi.colors.checkboxDarkFinish)

            gradient: Gradient {
                GradientStop {
                    position: 0.2
                    color: pressed || hovered
                           ? (checkBox.isLightColorScheme ? hifi.colors.checkboxChecked : hifi.colors.checkboxLightStart)
                           : (checkBox.isLightColorScheme ? hifi.colors.checkboxLightStart : hifi.colors.checkboxDarkStart)
                }
                GradientStop {
                    position: 1.0
                    color: pressed || hovered
                           ? (checkBox.isLightColorScheme ? hifi.colors.checkboxChecked : hifi.colors.checkboxLightFinish)
                           : (checkBox.isLightColorScheme ? hifi.colors.checkboxLightFinish : hifi.colors.checkboxDarkFinish)
                }
            }

            Rectangle {
                visible: pressed || hovered
                anchors.centerIn: parent
                id: innerBox
                width: checkSize - 4
                height: width
                radius: checkRadius
                color: hifi.colors.checkboxCheckedBorder
            }

            Rectangle {
                id: check
                width: checkSize
                height: checkSize
                radius: checkRadius
                anchors.centerIn: parent
                color: isRedCheck ? hifi.colors.checkboxCheckedRed : hifi.colors.checkboxChecked
                border.width: 2
                border.color: isRedCheck? hifi.colors.checkboxCheckedBorderRed : hifi.colors.checkboxCheckedBorder
                visible: checked && !pressed || !checked && pressed
            }

            Rectangle {
                id: disabledOverlay
                visible: !enabled
                width: boxSize
                height: boxSize
                radius: boxRadius
                border.width: 1
                border.color: hifi.colors.baseGrayHighlight
                color: hifi.colors.baseGrayHighlight
                opacity: 0.5
            }
        }

        label: Label {
            text: control.text
            color: control.color
            x: 2
            wrapMode: checkBox.wrap ? Text.Wrap : Text.NoWrap
            elide: checkBox.wrap ? Text.ElideNone : Text.ElideRight
            enabled: checkBox.enabled
        }
    }
}
