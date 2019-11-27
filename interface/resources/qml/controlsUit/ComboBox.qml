//
//  ComboBox.qml
//
//  Created by Bradley Austin David on 27 Jan 2016
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import QtQuick 2.7
import QtQuick.Controls 2.2

import "../stylesUit"
import "." as HifiControls

FocusScope {
    id: root
    HifiConstants { id: hifi }

    property alias model: comboBox.model;
    property alias editable: comboBox.editable
    property alias comboBox: comboBox
    readonly property alias currentText: comboBox.currentText;
    property alias displayText: comboBox.displayText;
    property alias currentIndex: comboBox.currentIndex;
    property int currentHighLightedIndex: comboBox.currentIndex;

    property int dropdownHeight: 480
    property int colorScheme: hifi.colorSchemes.light
    readonly property bool isLightColorScheme: colorScheme == hifi.colorSchemes.light
    property string label: ""
    property real controlHeight: height + (comboBoxLabel.visible ? comboBoxLabel.height + comboBoxLabel.anchors.bottomMargin : 0)

    readonly property ComboBox control: comboBox

    property bool isDesktop: true

    signal accepted();

    implicitHeight: comboBox.height;
    focus: true

    ComboBox {
        id: comboBox
        anchors.fill: parent
        hoverEnabled: true
        visible: true
        height: hifi.fontSizes.textFieldInput + 13  // Match height of TextField control.

        function previousItem() { root.currentHighLightedIndex = (root.currentHighLightedIndex + comboBox.count - 1) % comboBox.count; }
        function nextItem() { root.currentHighLightedIndex = (root.currentHighLightedIndex + comboBox.count + 1) % comboBox.count; }
        function selectCurrentItem() { root.currentIndex = root.currentHighLightedIndex; close(); /*hideList();*/ }
        function selectSpecificItem(index) { root.currentIndex = index; close();/*hideList();*/ }

        Keys.onUpPressed: previousItem();
        Keys.onDownPressed: nextItem();
        Keys.onSpacePressed: selectCurrentItem();
        Keys.onRightPressed: selectCurrentItem();
        Keys.onReturnPressed: selectCurrentItem();

        background: Rectangle {
            gradient: Gradient {
                GradientStop {
                    position: 0.2
                    color: comboBox.popup.visible
                           ? (isLightColorScheme ? hifi.colors.dropDownPressedLight : hifi.colors.dropDownPressedDark)
                           : (isLightColorScheme ? hifi.colors.dropDownLightStart : hifi.colors.dropDownDarkStart)
                }
                GradientStop {
                    position: 1.0
                    color: comboBox.popup.visible
                           ? (isLightColorScheme ? hifi.colors.dropDownPressedLight : hifi.colors.dropDownPressedDark)
                           : (isLightColorScheme ? hifi.colors.dropDownLightFinish : hifi.colors.dropDownDarkFinish)
                }
            }
        }

        indicator: Item {
            id: dropIcon
            anchors { right: parent.right; verticalCenter: parent.verticalCenter }
            height: root.height
            width: height
            Rectangle {
                width: 1
                height: parent.height
                anchors.top: parent.top
                anchors.left: parent.left
                color: isLightColorScheme ? hifi.colors.faintGray : hifi.colors.baseGray
            }
            HiFiGlyphs {
                anchors { top: parent.top; topMargin: -11; horizontalCenter: parent.horizontalCenter }
                size: hifi.dimensions.spinnerSize
                text: hifi.glyphs.caratDn
                color: comboBox.hovered || comboBox.popup.visible ? hifi.colors.baseGray : (isLightColorScheme ? hifi.colors.lightGray : hifi.colors.lightGrayText)
            }
        }

        contentItem: FiraSansSemiBold {
            id: textField
            anchors {
                left: parent.left
                leftMargin: hifi.dimensions.textPadding
                verticalCenter: parent.verticalCenter
            }
            size: hifi.fontSizes.textFieldInput
            text: comboBox.displayText ? comboBox.displayText : comboBox.currentText
            elide: Text.ElideRight
            color: comboBox.hovered || comboBox.popup.visible ? hifi.colors.baseGray : (isLightColorScheme ? hifi.colors.lightGray : hifi.colors.lightGrayText )
        }

        delegate: ItemDelegate {
            id: itemDelegate
            hoverEnabled: true
            width: root.width + 4
            height: popupText.implicitHeight * 1.4
            highlighted: root.currentHighLightedIndex == index

            onHoveredChanged: {
                if (hovered) {
                    root.currentHighLightedIndex = index
                }
            }

            background: Rectangle {
                color: itemDelegate.highlighted ? hifi.colors.primaryHighlight
                                                : (isLightColorScheme ? hifi.colors.dropDownPressedLight
                                                                      : hifi.colors.dropDownPressedDark)
            }

            contentItem: FiraSansSemiBold {
                id: popupText
                anchors.left: parent.left
                anchors.leftMargin: hifi.dimensions.textPadding
                anchors.verticalCenter: parent.verticalCenter
                text: comboBox.model[index] ? comboBox.model[index]
                                            : (comboBox.model.get && comboBox.model.get(index).text ?
                                                   comboBox.model.get(index).text : "")
                size: hifi.fontSizes.textFieldInput
                color: hifi.colors.baseGray
            }
        }
        popup: Popup {
            y: comboBox.height - 1
            width: comboBox.width
            implicitHeight: listView.contentHeight > dropdownHeight ? dropdownHeight
                                                                    : listView.contentHeight
            padding: 0
            topPadding: 1

            onClosed: {
                root.accepted()
            }

            contentItem: ListView {
                id: listView
                clip: true
                model: comboBox.popup.visible ? comboBox.delegateModel : null
                currentIndex: root.currentHighLightedIndex
                delegate: comboBox.delegate
                ScrollBar.vertical: HifiControls.ScrollBar {
                    id: scrollbar
                    parent: listView
                    policy: ScrollBar.AsNeeded
                    visible: size < 1.0
                }
            }

            background: Rectangle {
                color: hifi.colors.baseGray
            }
        }
    }

    function textAt(index) {
        return comboBox.textAt(index);
    }

    HifiControls.Label {
        id: comboBoxLabel
        text: root.label
        colorScheme: root.colorScheme
        anchors.left: parent.left
        anchors.bottom: parent.top
        anchors.bottomMargin: 4
        visible: label != ""
    }

    Component.onCompleted: {
        isDesktop = (typeof desktop !== "undefined");
    }
}
