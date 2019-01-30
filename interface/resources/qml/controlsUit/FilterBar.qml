//
//  FilterBar.qml
//
//  Created by Zach Fox on 17 Feb 2018-03-12
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import QtQuick 2.9
import QtQuick.Controls 2.2
import QtGraphicalEffects 1.0

import "../stylesUit"
import "." as HifiControls

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
    property int primaryFilter_index: -1;
    property string primaryFilter_filterName: "";
    property string primaryFilter_displayName: "";
    signal accepted;

    onPrimaryFilter_indexChanged: {
        if (primaryFilter_index === -1) {
            primaryFilter_filterName = "";
            primaryFilter_displayName = "";
        } else {
            primaryFilter_filterName = filterBarModel.get(primaryFilter_index).filterName;
            primaryFilter_displayName = filterBarModel.get(primaryFilter_index).displayName;
        }
    }

    TextField {
        id: textField;

        anchors.top: parent.top;
        anchors.right: parent.right;
        anchors.left: parent.left;

        font.family: "Fira Sans"
        font.pixelSize: hifi.fontSizes.textFieldInput;

        placeholderText: root.primaryFilter_index === -1 ? root.placeholderText : "";
                    
        TextMetrics {
            id: primaryFilterTextMetrics;
            font.family: "FiraSans Regular";
            font.pixelSize: hifi.fontSizes.textFieldInput;
            font.capitalization: Font.AllUppercase;
            text: root.primaryFilter_displayName;
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
                        root.forceActiveFocus();
                    }
                break;
                case Qt.Key_Backspace:
                    if (textField.text === "") {
                        primaryFilter_index = -1;
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

        color: {
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
            id: mainFilterBarRectangle;

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
            (textField.activeFocus ? hifi.colors.primaryHighlight : (isFaintGrayColorScheme ? hifi.colors.lightGrayText : hifi.colors.lightGray))
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
                    color: textField.color
                    size: 40;
                    anchors.left: parent.left
                    anchors.verticalCenter: parent.verticalCenter
                    width: paintedWidth;
                }

                // Carat
                HiFiGlyphs {
                    text: hifi.glyphs.caratDn;
                    color: textField.color;
                    size: 40;
                    anchors.left: parent.left;
                    anchors.leftMargin: 15;
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
                color: textField.activeFocus ? hifi.colors.faintGray : hifi.colors.white;
                width: primaryFilterTextMetrics.tightBoundingRect.width + 14;
                height: parent.height - 8;
                anchors.verticalCenter: parent.verticalCenter;
                anchors.left: searchButtonContainer.right;
                anchors.leftMargin: 4;
                visible: primaryFilterText.text !== "";
                radius: height/2;

                FiraSansRegular {
                    id: primaryFilterText;
                    text: root.primaryFilter_displayName;
                    anchors.fill: parent;
                    color: textField.activeFocus ? hifi.colors.black : hifi.colors.lightGray;
                    horizontalAlignment: Text.AlignHCenter;
                    verticalAlignment: Text.AlignVCenter;
                    size: hifi.fontSizes.textFieldInput;
                    font.capitalization: Font.AllUppercase;
                }

                MouseArea {
                    anchors.fill: parent;
                    onClicked: {
                        textField.forceActiveFocus();
                    }
                }
            }

            // "Clear" button
            HiFiGlyphs {
                text: hifi.glyphs.error
                color: textField.color
                size: 40
                anchors.right: parent.right
                anchors.rightMargin: hifi.dimensions.textPadding - 2
                anchors.verticalCenter: parent.verticalCenter
                visible: root.text !== "" || root.primaryFilter_index !== -1;

                MouseArea {
                    anchors.fill: parent;
                    onClicked: {
                        root.text = "";
                        root.primaryFilter_index = -1;
                        dropdownContainer.visible = false;
                        textField.forceActiveFocus();
                    }
                }
            }
        }

        selectedTextColor: hifi.colors.black
        selectionColor: hifi.colors.primaryHighlight
        leftPadding: 44 + (root.primaryFilter_index === -1 ? 0 : primaryFilterTextMetrics.tightBoundingRect.width + 20);
        rightPadding: 44;
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

        ListModel {
            id: filterBarModel;
        }

        ListView {
            id: dropdownListView;
            interactive: false;
            anchors.fill: parent;
            model: filterBarModel;
            delegate: Item {
                width: parent.width;
                height: 50;
                Rectangle {
                    id: dropDownButton;
                    color: hifi.colors.white;
                    width: parent.width;
                    height: 50;
                    visible: true;

                    RalewaySemiBold {
                        id: dropDownButtonText;
                        text: model.displayName;
                        anchors.fill: parent;
                        anchors.topMargin: 2;
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
                            root.primaryFilter_index = index;
                            dropdownContainer.visible = false;
                        }
                    }
                }
                Rectangle {
                    height: 2;
                    width: parent.width;
                    color: hifi.colors.lightGray;
                    visible: model.separator
                }
            }
        }
    }

    DropShadow {
        anchors.fill: dropdownContainer;
        horizontalOffset: 0;
        verticalOffset: 4;
        radius: 4.0;
        samples: 9
        color: Qt.rgba(0, 0, 0, 0.25);
        source: dropdownContainer;
        visible: dropdownContainer.visible;
    }

    function changeFilterByDisplayName(name) {
        for (var i = 0; i < filterBarModel.count; i++) {
            if (filterBarModel.get(i).displayName === name) {
                root.primaryFilter_index = i;
                return;
            }
        }

        console.log("Passed displayName not found in filterBarModel! primaryFilter unchanged.");
    }
}
