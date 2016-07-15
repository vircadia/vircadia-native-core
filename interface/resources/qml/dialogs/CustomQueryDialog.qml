//
//  CustomQueryDialog.qml
//
//  Created by Zander Otavka on 7/14/16
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import QtQuick 2.5;
import QtQuick.Controls 1.4;
import QtQuick.Dialogs 1.2 as OriginalDialogs;

import "../controls-uit";
import "../styles-uit";
import "../windows";

ModalWindow {
    id: root;
    HifiConstants { id: hifi; }
    implicitWidth: 640;
    implicitHeight: 320;
    visible: true;

    signal selected(var result);
    signal canceled();

    property int icon: hifi.icons.none;
    property string iconText: "";
    property int iconSize: 35;
    onIconChanged: updateIcon();

    property var textInput;
    property var comboBox;
    property var checkBox;

    property var result;
    property alias current: textField.text;

    // For text boxes
    property alias placeholderText: textField.placeholderText;

    // For combo boxes
    property bool editable: true;

    property int titleWidth: 0;
    onTitleWidthChanged: d.resize();

    function updateIcon() {
        if (!root) {
            return;
        }
        iconText = hifi.glyphForIcon(root.icon);
    }

    Item {
        clip: true;
        width: pane.width;
        height: pane.height;
        anchors.margins: 0;

        QtObject {
            id: d;
            readonly property int minWidth: 480;
            readonly property int maxWdith: 1280;
            readonly property int minHeight: 120;
            readonly property int maxHeight: 720;

            function resize() {
                var targetWidth = Math.max(titleWidth, pane.width);
                var targetHeight = (textInput ? textField.controlHeight : 0) + extraInputs.height +
                                   (6 * hifi.dimensions.contentSpacing.y) + buttons.height;
                // var targetHeight = 720;
                print("CQD extraInputs.height = " + extraInputs.height);
                print("CQD textField.controlHeight = " + textField.controlHeight);
                print("CQD buttons.height = " + buttons.height);
                print("CQD targetHeight = " + targetHeight);
                root.width = (targetWidth < d.minWidth) ? d.minWidth : ((targetWidth > d.maxWdith) ? d.maxWidth : targetWidth);
                root.height = (targetHeight < d.minHeight) ? d.minHeight: ((targetHeight > d.maxHeight) ?
                              d.maxHeight : targetHeight);
                print("CQD comboBoxField.visible = " + comboBoxField.visible);
                if (comboBoxField.visible) {
                    print("CQD parent = " + parent);
                    comboBoxField.width = extraInputs.width / 2;
                }
            }
        }

        Item {
            anchors {
                top: parent.top;
                bottom: extraInputs.top;
                left: parent.left;
                right: parent.right;
                margins: 0;
                bottomMargin: 2 * hifi.dimensions.contentSpacing.y;
            }

            // FIXME make a text field type that can be bound to a history for autocompletion
            TextField {
                id: textField;
                label: root.textInput.label;
                focus: root.textInput ? true : false;
                visible: root.textInput ? true : false;
                anchors {
                    left: parent.left;
                    right: parent.right;
                    bottom: parent.bottom;
                }
            }
        }

        Row {
            id: extraInputs;
            spacing: hifi.dimensions.contentSpacing.x;
            anchors {
                left: parent.left;
                right: parent.right;
                bottom: buttons.top;
                bottomMargin: hifi.dimensions.contentSpacing.y;
            }
            height: comboBoxField.controlHeight;
            onHeightChanged: d.resize();
            onWidthChanged: d.resize();

            CheckBox {
                id: checkBoxField;
                text: root.checkBox.label;
                focus: root.checkBox ? true : false;
                visible: root.checkBox ? true : false;
                anchors {
                    left: parent.left;
                    bottom: parent.bottom;
                }
            }

            ComboBox {
                id: comboBoxField;
                label: root.comboBox.label;
                focus: root.comboBox ? true : false;
                visible: root.comboBox ? true : false;
                anchors {
                    right: parent.right;
                    bottom: parent.bottom;
                }
                model: root.comboBox ? root.comboBox.items : [];
            }
        }

        Row {
            id: buttons;
            focus: true;
            spacing: hifi.dimensions.contentSpacing.x;
            onHeightChanged: d.resize();
            onWidthChanged: d.resize();
            anchors {
                bottom: parent.bottom;
                left: parent.left;
                right: parent.right;
                bottomMargin: hifi.dimensions.contentSpacing.y;
            }

            Button {
                id: acceptButton;
                action: acceptAction;
                // anchors {
                //     bottom: parent.bottom;
                //     right: cancelButton.left;
                //     leftMargin: hifi.dimensions.contentSpacing.x;
                // }
            }

            Button {
                id: cancelButton;
                action: cancelAction;
                // anchors {
                //     bottom: parent.bottom;
                //     right: parent.right;
                //     leftMargin: hifi.dimensions.contentSpacing.x;
                // }
            }
        }

        Action {
            id: cancelAction;
            text: qsTr("Cancel");
            shortcut: Qt.Key_Escape;
            onTriggered: {
                root.result = null;
                root.canceled();
                root.destroy();
            }
        }

        Action {
            id: acceptAction;
            text: qsTr("Add");
            shortcut: Qt.Key_Return;
            onTriggered: {
                root.result = {
                    textInput: textField.text,
                    comboBox: comboBoxField.currentText,
                    checkBox: checkBoxField.checked
                };
                root.selected(root.result);
                root.destroy();
            }
        }
    }

    Keys.onPressed: {
        if (!visible) {
            return;
        }

        switch (event.key) {
            case Qt.Key_Escape:
            case Qt.Key_Back:
                cancelAction.trigger();
                event.accepted = true;
                break;

            case Qt.Key_Return:
            case Qt.Key_Enter:
                acceptAction.trigger();
                event.accepted = true;
                break;
        }
    }

    Component.onCompleted: {
        updateIcon();
        d.resize();
        textField.forceActiveFocus();
    }
}
