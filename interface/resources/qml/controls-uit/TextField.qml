//
//  TextField.qml
//
//  Created by David Rowe on 17 Feb 2016
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

TextField {
    id: textField

    property int colorScheme: hifi.colorSchemes.light
    readonly property bool isLightColorScheme: colorScheme == hifi.colorSchemes.light
    property bool isSearchField: false
    property string label: ""
    property real controlHeight: height + (textFieldLabel.visible ? textFieldLabel.height + 1 : 0)

    placeholderText: textField.placeholderText

    FontLoader { id: firaSansSemiBold; source: "../../fonts/FiraSans-SemiBold.ttf"; }
    font.family: firaSansSemiBold.name
    font.pixelSize: hifi.fontSizes.textFieldInput
    font.italic: textField.text == ""
    height: implicitHeight + 3  // Make surrounding box higher so that highlight is vertically centered.

    y: textFieldLabel.visible ? textFieldLabel.height + textFieldLabel.anchors.bottomMargin : 0

    style: TextFieldStyle {
        textColor: isLightColorScheme
                   ? (textField.activeFocus ? hifi.colors.black : hifi.colors.lightGray)
                   : (textField.activeFocus ? hifi.colors.white : hifi.colors.lightGrayText)
        background: Rectangle {
            color: isLightColorScheme
                   ? (textField.activeFocus ? hifi.colors.white : hifi.colors.textFieldLightBackground)
                   : (textField.activeFocus ? hifi.colors.black : hifi.colors.baseGrayShadow)
            border.color: hifi.colors.primaryHighlight
            border.width: textField.activeFocus ? 1 : 0
            radius: isSearchField ? textField.height / 2 : 0

            HiFiGlyphs {
                text: hifi.glyphs.search
                color: textColor
                size: hifi.fontSizes.textFieldSearchIcon
                anchors.left: parent.left
                anchors.verticalCenter: parent.verticalCenter
                anchors.leftMargin: hifi.dimensions.textPadding - 2
                visible: isSearchField
            }
        }
        placeholderTextColor: hifi.colors.lightGray
        selectedTextColor: hifi.colors.black
        selectionColor: hifi.colors.primaryHighlight
        padding.left: (isSearchField ? textField.height - 2 : 0) + hifi.dimensions.textPadding
        padding.right: hifi.dimensions.textPadding
    }

    HifiControls.Label {
        id: textFieldLabel
        text: textField.label
        colorScheme: textField.colorScheme
        anchors.left: parent.left
        anchors.bottom: parent.top
        anchors.bottomMargin: 3
        visible: label != ""
    }
}
