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
    property string label: ""
    property real controlHeight: height + (textFieldLabel.visible ? textFieldLabel.height : 0)

    placeholderText: textField.placeholderText

    FontLoader { id: firaSansSemiBold; source: "../../fonts/FiraSans-SemiBold.ttf"; }
    font.family: firaSansSemiBold.name
    font.pixelSize: hifi.fontSizes.textFieldInput
    height: implicitHeight + 4  // Make surrounding box higher so that highlight is vertically centered.

    y: textFieldLabel.visible ? textFieldLabel.height + textFieldLabel.anchors.bottomMargin : 0

    style: TextFieldStyle {
        textColor: isLightColorScheme
                   ? (textField.focus ? hifi.colors.black : hifi.colors.lightGray)
                   : (textField.focus ? hifi.colors.white : hifi.colors.lightGrayText)
        background: Rectangle {
            color: isLightColorScheme
                   ? (textField.focus ? hifi.colors.white : hifi.colors.lightGray)
                   : (textField.focus ? hifi.colors.black : hifi.colors.baseGrayShadow)
            border.color: hifi.colors.primaryHighlight
            border.width: textField.focus ? 1 : 0
        }
        placeholderTextColor: hifi.colors.lightGray
        selectedTextColor: hifi.colors.black
        selectionColor: hifi.colors.primaryHighlight
        padding.left: hifi.dimensions.textPadding
        padding.right: hifi.dimensions.textPadding
    }

    HifiControls.Label {
        id: textFieldLabel
        text: textField.label
        colorScheme: textField.colorScheme
        anchors.left: parent.left
        anchors.bottom: parent.top
        anchors.bottomMargin: 4
        visible: label != ""
    }
}
