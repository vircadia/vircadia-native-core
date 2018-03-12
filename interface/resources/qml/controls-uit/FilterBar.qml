//
//  FilterBar.qml
//
//  Created by Zach Fox on 17 Feb 2018-03-12
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import QtQuick 2.5
import QtQuick.Controls 1.4
import QtQuick.Controls.Styles 1.4

import "../styles-uit"
import "../controls-uit" as HifiControls

Item {
    id: root;

    property int colorScheme: hifi.colorSchemes.light
    readonly property bool isLightColorScheme: colorScheme == hifi.colorSchemes.light
    readonly property bool isFaintGrayColorScheme: colorScheme == hifi.colorSchemes.faintGray
    property bool error: false;
    property alias textFieldHeight: textField.height;
    property string placeholderText;
    property alias dropdownHeight: dropdownContainer.height;
    property alias text: textField.text;
    property alias primaryFilterChoices: filterBarModel;
    property alias textFieldFocused: textField.focus;
    property string primaryFilter: "";
    signal accepted;

    TextField {
        id: textField;

        anchors.top: parent.top;
        anchors.right: parent.right;
        anchors.left: parent.left;

        font.family: "Fira Sans"
        font.pixelSize: hifi.fontSizes.textFieldInput

        placeholderText: root.primaryFilter === "" ? root.placeholderText : "";
                    
        TextMetrics {
            id: primaryFilterTextMetrics;
            font.family: "FiraSans Regular";
            text: root.primaryFilter;
        }

        // workaround for https://bugreports.qt.io/browse/QTBUG-49297
        Keys.onPressed: {
            switch (event.key) {
                case Qt.Key_Return: 
                case Qt.Key_Enter: 
                    event.accepted = true;

                    // emit accepted signal manually
                    if (acceptableInput) {
                        root.accepted();
                    }
                break;
                case Qt.Key_Backspace:
                    if (textField.text === "") {
                        root.primaryFilter = "";
                    }
                break;
            }
        }

        onAccepted: {
            root.forceActiveFocus();
        }

        onActiveFocusChanged: {
            if (!activeFocus) {
                dropdownContainer.visible = false;
            }
        }

        style: TextFieldStyle {
            id: style;
            textColor: {
                if (isLightColorScheme) {
                    if (root.activeFocus) {
                        hifi.colors.black
                    } else {
                        hifi.colors.lightGray
                    }
                } else if (isFaintGrayColorScheme) {
                    if (root.activeFocus) {
                        hifi.colors.black
                    } else {
                        hifi.colors.lightGray
                    }
                } else {
                    if (root.activeFocus) {
                        hifi.colors.white
                    } else {
                        hifi.colors.lightGrayText
                    }
                }
            }
            background: Rectangle {
                id: mainFilterBarRectangle;

                color: {
                    if (isLightColorScheme) {
                        if (root.activeFocus) {
                            hifi.colors.white
                        } else {
                            hifi.colors.textFieldLightBackground
                        }
                    } else if (isFaintGrayColorScheme) {
                        if (root.activeFocus) {
                            hifi.colors.white
                        } else {
                            hifi.colors.faintGray50
                        }
                    } else {
                        if (root.activeFocus) {
                            hifi.colors.black
                        } else {
                            hifi.colors.baseGrayShadow
                        }
                    }
                }

                border.color: root.error ? hifi.colors.redHighlight :
                (root.activeFocus ? hifi.colors.primaryHighlight : (isFaintGrayColorScheme ? hifi.colors.lightGrayText : hifi.colors.lightGray))
                border.width: 1
                radius: 4

                Item {
                    id: searchButtonContainer;
                    anchors.left: parent.left;
                    anchors.verticalCenter: parent.verticalCenter;
                    height: parent.height;
                    width: 42;

                    // Search icon
                    HiFiGlyphs {
                        id: searchIcon;
                        text: hifi.glyphs.search
                        color: textColor
                        size: 40;
                        anchors.left: parent.left
                        anchors.verticalCenter: parent.verticalCenter
                        width: paintedWidth;
                    }

                    // Carat
                    HiFiGlyphs {
                        text: hifi.glyphs.caratDn;
                        color: textColor;
                        size: 40;
                        anchors.left: parent.left;
                        anchors.leftMargin: 14;
                        width: paintedWidth;
                    }

                    MouseArea {
                        anchors.fill: parent;
                        onClicked: {
                            textField.forceActiveFocus();
                            dropdownContainer.visible = !dropdownContainer.visible;
                        }
                    }
                }

                Rectangle {
                    z: 999;
                    id: primaryFilterContainer;
                    color: hifi.colors.lightGray;
                    width: primaryFilterTextMetrics.tightBoundingRect.width + 24;
                    height: parent.height - 8;
                    anchors.verticalCenter: parent.verticalCenter;
                    anchors.left: searchButtonContainer.right;
                    anchors.leftMargin: 4;
                    visible: primaryFilterText.text !== "";

                    FiraSansRegular {
                        id: primaryFilterText;
                        text: root.primaryFilter;
                        anchors.fill: parent;
                        color: hifi.colors.white;
                        horizontalAlignment: Text.AlignHCenter;
                        verticalAlignment: Text.AlignVCenter;
                        size: 16;
                    }
                }

                // "Clear" button
                HiFiGlyphs {
                    text: hifi.glyphs.error
                    color: textColor
                    size: 40
                    anchors.right: parent.right
                    anchors.rightMargin: hifi.dimensions.textPadding - 2
                    anchors.verticalCenter: parent.verticalCenter
                    visible: root.text !== "" || root.primaryFilter !== "";

                    MouseArea {
                        anchors.fill: parent;
                        onClicked: {
                            root.text = "";
                            root.primaryFilter = "";
                            root.forceActiveFocus();
                        }
                    }
                }
            }

            placeholderTextColor: isFaintGrayColorScheme ? hifi.colors.lightGrayText : hifi.colors.lightGray
            selectedTextColor: hifi.colors.black
            selectionColor: hifi.colors.primaryHighlight
            padding.left: 44 + (root.primaryFilter === "" ? 0 : primaryFilterTextMetrics.tightBoundingRect.width + 32);
            padding.right: 44
        }
    }

    Rectangle {
        id: dropdownContainer;
        visible: false;
        height: 50 * filterBarModel.count;
        width: parent.width;
        anchors.top: textField.bottom;
        anchors.left: parent.left;
        anchors.right: parent.right;
        color: hifi.colors.white;
        signal buttonClicked(string text);

        onButtonClicked: {
            root.primaryFilter = text;
        }

        ListModel {
            id: filterBarModel;
        }

        ListView {
            anchors.fill: parent;
            model: filterBarModel;
            delegate: Rectangle {
                id: dropDownButton;
                color: hifi.colors.white;
                width: parent.width;
                height: 50;

                RalewaySemiBold {
                    id: dropDownButtonText;
                    text: model.displayName;
                    anchors.fill: parent;
                    anchors.leftMargin: 12;
                    color: hifi.colors.baseGray;
                    horizontalAlignment: Text.AlignLeft;
                    verticalAlignment: Text.AlignVCenter;
                    size: 18;
                }

                MouseArea {
                    anchors.fill: parent;
                    hoverEnabled: true;
                    propagateComposedEvents: false;
                    onEntered: {
                        dropDownButton.color = hifi.colors.blueHighlight;
                    }
                    onExited: {
                        dropDownButton.color = hifi.colors.white;
                    }
                    onClicked: {
                        textField.forceActiveFocus();
                        dropdownContainer.buttonClicked(model.filterName);
                        dropdownContainer.visible = false;
                    }
                }
            }
        }
    }
}
