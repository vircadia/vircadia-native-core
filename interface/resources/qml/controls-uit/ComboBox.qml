//
//  ComboBox.qml
//
//  Created by Bradley Austin David on 27 Jan 2016
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
import "." as VrControls

FocusScope {
    id: root

    property alias model: comboBox.model;
    property alias comboBox: comboBox
    readonly property alias currentText: comboBox.currentText;
    property alias currentIndex: comboBox.currentIndex;

    property int colorScheme: hifi.colorSchemes.light
    readonly property bool isLightColorScheme: colorScheme == hifi.colorSchemes.light
    property string label: ""
    property real controlHeight: height + (comboBoxLabel.visible ? comboBoxLabel.height + comboBoxLabel.anchors.bottomMargin : 0)

    readonly property ComboBox control: comboBox

    implicitHeight: comboBox.height;
    focus: true

    Rectangle {
        id: background
        gradient: Gradient {
            GradientStop {
                position: 0.2
                color: popup.visible
                       ? (isLightColorScheme ? hifi.colors.dropDownPressedLight : hifi.colors.dropDownPressedDark)
                       : (isLightColorScheme ? hifi.colors.dropDownLightStart : hifi.colors.dropDownDarkStart)
            }
            GradientStop {
                position: 1.0
                color: popup.visible
                       ? (isLightColorScheme ? hifi.colors.dropDownPressedLight : hifi.colors.dropDownPressedDark)
                       : (isLightColorScheme ? hifi.colors.dropDownLightFinish : hifi.colors.dropDownDarkFinish)
            }
        }
        anchors.fill: parent
    }

    SystemPalette { id: palette }

    ComboBox {
        id: comboBox
        anchors.fill: parent
        visible: false
        height: hifi.fontSizes.textFieldInput + 13  // Match height of TextField control.
    }

    FiraSansSemiBold {
        id: textField
        anchors {
            left: parent.left
            leftMargin: hifi.dimensions.textPadding
            right: dropIcon.left
            verticalCenter: parent.verticalCenter
        }
        size: hifi.fontSizes.textFieldInput
        text: comboBox.currentText
        elide: Text.ElideRight
        color: controlHover.containsMouse || popup.visible ? hifi.colors.baseGray : (isLightColorScheme ? hifi.colors.lightGray : hifi.colors.lightGrayText )
    }

    Item {
        id: dropIcon
        anchors { right: parent.right; verticalCenter: parent.verticalCenter }
        height: background.height
        width: height
        Rectangle {
            width: 1
            height: parent.height
            anchors.top: parent.top
            anchors.left: parent.left
            color: isLightColorScheme ? hifi.colors.faintGray : hifi.colors.baseGray
        }
        HiFiGlyphs {
            anchors {
                top: parent.top
                topMargin: -11
                horizontalCenter: parent.horizontalCenter
            }
            size: hifi.dimensions.spinnerSize
            text: hifi.glyphs.caratDn
            color: controlHover.containsMouse || popup.visible ? hifi.colors.baseGray : (isLightColorScheme ? hifi.colors.lightGray : hifi.colors.lightGrayText)
        }
    }

    MouseArea {
        id: controlHover
        hoverEnabled: true
        anchors.fill: parent
        onClicked: toggleList();
    }

    function toggleList() {
        if (popup.visible) {
            hideList();
        } else {
            showList();
        }
    }

    function showList() {
        var r = desktop.mapFromItem(root, 0, 0, root.width, root.height);
        listView.currentIndex = root.currentIndex
        scrollView.x = r.x;
        scrollView.y = r.y + r.height;
        var bottom = scrollView.y + scrollView.height;
        if (bottom > desktop.height) {
            scrollView.y -= bottom - desktop.height + 8;
        }
        popup.visible = true;
        popup.forceActiveFocus();
    }

    function hideList() {
        popup.visible = false;
    }

    FocusScope {
        id: popup
        parent: desktop
        anchors.fill: parent
        z: desktop.zLevels.menu
        visible: false
        focus: true

        MouseArea {
            anchors.fill: parent
            onClicked: hideList();
        }

        function previousItem() { listView.currentIndex = (listView.currentIndex + listView.count - 1) % listView.count; }
        function nextItem() { listView.currentIndex = (listView.currentIndex + listView.count + 1) % listView.count; }
        function selectCurrentItem() { root.currentIndex = listView.currentIndex; hideList(); }
        function selectSpecificItem(index) { root.currentIndex = index; hideList(); }

        Keys.onUpPressed: previousItem();
        Keys.onDownPressed: nextItem();
        Keys.onSpacePressed: selectCurrentItem();
        Keys.onRightPressed: selectCurrentItem();
        Keys.onReturnPressed: selectCurrentItem();
        Keys.onEscapePressed: hideList();

        ScrollView {
            id: scrollView
            height: 480
            width: root.width + 4

            style: ScrollViewStyle {
                decrementControl: Item {
                    visible: false
                }
                incrementControl: Item {
                    visible: false
                }
                scrollBarBackground: Rectangle{
                    implicitWidth: 14
                    color: hifi.colors.baseGray
                }

                handle:
                    Rectangle {
                    implicitWidth: 8
                    anchors.left: parent.left
                    anchors.leftMargin: 3
                    anchors.top: parent.top
                    anchors.bottom: parent.bottom
                    radius: 3
                    color: hifi.colors.lightGrayText
                }
            }

            ListView {
                id: listView
                height: textField.height * count * 1.4
                model: root.model
                delegate: Rectangle {
                    width: root.width + 4
                    height: popupText.implicitHeight * 1.4
                    color: popupHover.containsMouse ? hifi.colors.primaryHighlight : (isLightColorScheme ? hifi.colors.dropDownPressedLight : hifi.colors.dropDownPressedDark)
                    FiraSansSemiBold {
                        anchors.left: parent.left
                        anchors.leftMargin: hifi.dimensions.textPadding
                        anchors.verticalCenter: parent.verticalCenter
                        id: popupText
                        text: listView.model[index] ? listView.model[index] : ""
                        size: hifi.fontSizes.textFieldInput
                        color: hifi.colors.baseGray
                    }
                    MouseArea {
                        id: popupHover
                        anchors.fill: parent;
                        hoverEnabled: true
                        onEntered: listView.currentIndex = index;
                        onClicked: popup.selectSpecificItem(index)
                    }
                }
            }
        }
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
}
