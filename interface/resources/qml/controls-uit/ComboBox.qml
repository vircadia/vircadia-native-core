//
//  ComboBox.qml
//
//  Created by David Rowe on 27 Feb 2016
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

// FIXME: Currently supports only the "Drop Down Box" case in the UI Toolkit doc;
// Should either be made to also support the "Combo Box" case or drop-down and combo box should be separate controls.

// FIXME: Style dropped-down items per UI Toolkit Drop Down Box.
// http://stackoverflow.com/questions/27089779/qml-combobox-item-dropdownmenu-style/27217209#27217209

ComboBox {
    id: comboBox

    property int colorScheme: hifi.colorSchemes.light
    readonly property bool isLightColorScheme: colorScheme == hifi.colorSchemes.light
    property string label: ""
    property real controlHeight: height + (comboBoxLabel.visible ? comboBoxLabel.height : 0)

    height: hifi.fontSizes.textFieldInput + 14  // Match height of TextField control.

    y: comboBoxLabel.visible ? comboBoxLabel.height : 0

    style: ComboBoxStyle {
        FontLoader { id: firaSansSemiBold; source: "../../fonts/FiraSans-SemiBold.ttf"; }
        font {
            family: firaSansSemiBold.name
            pixelSize: hifi.fontSizes.textFieldInput
        }

        background: Rectangle {
            gradient: Gradient {
                GradientStop {
                    position: 0.2
                    color: pressed
                           ? (isLightColorScheme ? hifi.colors.dropDownPressedLight : hifi.colors.dropDownPressedDark)
                           : (isLightColorScheme ? hifi.colors.dropDownLightStart : hifi.colors.dropDownDarkStart)
                }
                GradientStop {
                    position: 1.0
                    color: pressed
                           ? (isLightColorScheme ? hifi.colors.dropDownPressedLight : hifi.colors.dropDownPressedDark)
                           : (isLightColorScheme ? hifi.colors.dropDownLightFinish : hifi.colors.dropDownDarkFinish)
                }
            }

            HiFiGlyphs {
                text: hifi.glyphs.backward  // Adapt backward triangle to be downward triangle
                rotation: -90
                y: -2
                size: hifi.dimensions.spinnerSize
                color: pressed ? (isLightColorScheme ? hifi.colors.black : hifi.colors.white) : hifi.colors.baseGray
                anchors {
                    right: parent.right
                    rightMargin: 3
                    verticalCenter: parent.verticalCenter
                }
            }

            Rectangle {
                width: 1
                height: parent.height
                color: isLightColorScheme ? hifi.colors.faintGray : hifi.colors.baseGray
                anchors {
                    top: parent.top
                    right: parent.right
                    rightMargin: parent.height
                }
            }
        }

        textColor: pressed ? hifi.colors.baseGray : (isLightColorScheme ? hifi.colors.lightGray : hifi.colors.lightGrayText )
        selectedTextColor: hifi.colors.baseGray
        selectionColor: hifi.colors.primaryHighlight
    }

    HifiControls.Label {
        id: comboBoxLabel
        text: comboBox.label
        colorScheme: comboBox.colorScheme
        anchors.left: parent.left
        anchors.bottom: parent.top
        anchors.bottomMargin: 4
        visible: label != ""
    }
}
