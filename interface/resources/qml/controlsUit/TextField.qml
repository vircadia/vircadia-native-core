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

import "../stylesUit"
import "." as HifiControls

TextField {
    id: textField

    property int colorScheme: hifi.colorSchemes.light
    readonly property bool isLightColorScheme: colorScheme == hifi.colorSchemes.light
    readonly property bool isFaintGrayColorScheme: colorScheme == hifi.colorSchemes.faintGray
    property bool isSearchField: false
    property string label: ""
    property real controlHeight: height + (textFieldLabel.visible ? textFieldLabel.height + 1 : 0)
    property bool hasDefocusedBorder: true;
    property bool hasRoundedBorder: false
    property int roundedBorderRadius: 4
    property bool error: false;
    property bool hasClearButton: false;
    property string leftPermanentGlyph: "";
    property string centerPlaceholderGlyph: "";

    placeholderText: textField.placeholderText

    font.family: "Fira Sans"
    font.pixelSize: hifi.fontSizes.textFieldInput
    height: implicitHeight + 3  // Make surrounding box higher so that highlight is vertically centered.
    property alias textFieldLabel: textFieldLabel

    y: textFieldLabel.visible ? textFieldLabel.height + textFieldLabel.anchors.bottomMargin : 0

    // workaround for https://bugreports.qt.io/browse/QTBUG-49297
    Keys.onPressed: {
        switch (event.key) {
            case Qt.Key_Return: 
            case Qt.Key_Enter: 
                event.accepted = true;

                // emit accepted signal manually
                if (acceptableInput) {
                    accepted();
                }
        }
    }

    style: TextFieldStyle {
        id: style;
        textColor: {
            if (isLightColorScheme) {
                if (textField.activeFocus) {
                    hifi.colors.black
                } else {
                    hifi.colors.lightGray
                }
            } else if (isFaintGrayColorScheme) {
                if (textField.activeFocus) {
                    hifi.colors.black
                } else {
                    hifi.colors.lightGray
                }
            } else {
                if (textField.activeFocus) {
                    hifi.colors.white
                } else {
                    hifi.colors.lightGrayText
                }
            }
        }
        background: Rectangle {
            color: {
            if (isLightColorScheme) {
                if (textField.activeFocus) {
                    hifi.colors.white
                } else {
                    hifi.colors.textFieldLightBackground
                }
            } else if (isFaintGrayColorScheme) {
                if (textField.activeFocus) {
                    hifi.colors.white
                } else {
                    hifi.colors.faintGray50
                }
            } else {
                if (textField.activeFocus) {
                    hifi.colors.black
                } else {
                    hifi.colors.baseGrayShadow
                }
            }
        }
            border.color: textField.error ? hifi.colors.redHighlight :
            (textField.activeFocus ? hifi.colors.primaryHighlight : (hasDefocusedBorder ? (isFaintGrayColorScheme ? hifi.colors.lightGrayText : hifi.colors.lightGray) : color))
            border.width: textField.activeFocus || hasRoundedBorder || textField.error ? 1 : 0
            radius: isSearchField ? textField.height / 2 : (hasRoundedBorder ? roundedBorderRadius : 0)

            HiFiGlyphs {
                text: textField.leftPermanentGlyph;
                color: textColor;
                size: hifi.fontSizes.textFieldSearchIcon;
                anchors.left: parent.left;
                anchors.verticalCenter: parent.verticalCenter;
                anchors.leftMargin: hifi.dimensions.textPadding - 2;
                visible: text;
            }

            HiFiGlyphs {
                text: textField.centerPlaceholderGlyph;
                color: textColor;
                size: parent.height;
                anchors.horizontalCenter: parent.horizontalCenter;
                anchors.verticalCenter: parent.verticalCenter;
                visible: text && !textField.focus && textField.text === "";
            }

            HiFiGlyphs {
                text: hifi.glyphs.search
                color: textColor
                size: hifi.fontSizes.textFieldSearchIcon
                anchors.left: parent.left
                anchors.verticalCenter: parent.verticalCenter
                anchors.leftMargin: hifi.dimensions.textPadding - 2
                visible: isSearchField
            }

            HiFiGlyphs {
                text: hifi.glyphs.error
                color: textColor
                size: 40
                anchors.right: parent.right
                anchors.rightMargin: hifi.dimensions.textPadding - 2
                anchors.verticalCenter: parent.verticalCenter
                visible: hasClearButton && textField.text !== "";

                MouseArea {
                    anchors.fill: parent;
                    onClicked: {
                        textField.text = "";
                    }
                }
            }
        }
        placeholderTextColor: isFaintGrayColorScheme ? hifi.colors.lightGrayText : hifi.colors.lightGray
        selectedTextColor: hifi.colors.black
        selectionColor: hifi.colors.primaryHighlight
        padding.left: hasRoundedBorder ? textField.height / 2 : ((isSearchField || textField.leftPermanentGlyph !== "") ? textField.height - 2 : 0) + hifi.dimensions.textPadding
        padding.right: (hasClearButton ? textField.height - 2 : 0) + hifi.dimensions.textPadding
    }

    HifiControls.Label {
        id: textFieldLabel
        text: textField.label
        colorScheme: textField.colorScheme
        anchors.left: parent.left

        Binding on anchors.right {
            when: textField.right
            value: textField.right
        }
        Binding on wrapMode {
            when: textField.right
            value: Text.WordWrap
        }

        anchors.bottom: parent.top
        anchors.bottomMargin: 3
        visible: label != ""
    }
}
