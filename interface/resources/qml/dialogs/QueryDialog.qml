//
//  QueryDialog.qml
//
//  Created by Bradley Austin Davis on 22 Jan 2016
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import QtQuick 2.7
import QtQuick.Controls 2.3

import "../controls-uit"
import "../styles-uit"
import "../windows"

ModalWindow {
    id: root
    HifiConstants { id: hifi }
    implicitWidth: 640
    implicitHeight: 320
    visible: true
    keyboardOverride: true  // Disable ModalWindow's keyboard.

    signal selected(var result);
    signal canceled();

    property int icon: hifi.icons.none
    property string iconText: ""
    property int iconSize: 35
    onIconChanged: updateIcon();

    property var items;
    property string label
    property var result;
    property alias current: textResult.text

    // For text boxes
    property alias placeholderText: textResult.placeholderText

    // For combo boxes
    property bool editable: true;

    property int titleWidth: 0
    onTitleWidthChanged: d.resize();

    property bool keyboardEnabled: false
    property bool keyboardRaised: false
    property bool punctuationMode: false

    onKeyboardRaisedChanged: d.resize();

    function updateIcon() {
        if (!root) {
            return;
        }
        iconText = hifi.glyphForIcon(root.icon);
    }

    Item {
        id: modalWindowItem
        clip: true
        width: pane.width
        height: pane.height
        anchors.margins: 0

        QtObject {
            id: d
            readonly property int minWidth: 480
            readonly property int maxWdith: 1280
            readonly property int minHeight: 120
            readonly property int maxHeight: 720

            function resize() {
                var targetWidth = Math.max(titleWidth, pane.width)
                var targetHeight = (items ? comboBox.controlHeight : textResult.controlHeight) + 5 * hifi.dimensions.contentSpacing.y + buttons.height
                root.width = (targetWidth < d.minWidth) ? d.minWidth : ((targetWidth > d.maxWdith) ? d.maxWidth : targetWidth);
                root.height = ((targetHeight < d.minHeight) ? d.minHeight : ((targetHeight > d.maxHeight) ? d.maxHeight : targetHeight)) + ((keyboardEnabled && keyboardRaised) ? (keyboard.raisedHeight + 2 * hifi.dimensions.contentSpacing.y) : 0)
            }
        }

        Item {
            anchors {
                top: parent.top
                bottom: keyboard.top;
                left: parent.left;
                right: parent.right;
                margins: 0
                bottomMargin: 2 * hifi.dimensions.contentSpacing.y
            }

            // FIXME make a text field type that can be bound to a history for autocompletion
            TextField {
                id: textResult
                label: root.label
                visible: items ? false : true
                anchors {
                    left: parent.left;
                    right: parent.right;
                    bottom: parent.bottom
                }
                KeyNavigation.down: acceptButton
                KeyNavigation.tab: acceptButton
            }

            ComboBox {
                id: comboBox
                label: root.label
                visible: items ? true : false
                anchors {
                    left: parent.left
                    right: parent.right
                    bottom: parent.bottom
                }
                model: items ? items : []
                KeyNavigation.down: acceptButton
                KeyNavigation.tab: acceptButton
            }
        }

        property alias keyboardOverride: root.keyboardOverride
        property alias keyboardRaised: root.keyboardRaised
        property alias punctuationMode: root.punctuationMode
        Keyboard {
            id: keyboard
            raised: keyboardEnabled && keyboardRaised
            numeric: punctuationMode
            anchors {
                left: parent.left
                right: parent.right
                bottom: buttons.top
                bottomMargin: raised ? 2 * hifi.dimensions.contentSpacing.y : 0
            }
        }

        Flow {
            id: buttons
            spacing: hifi.dimensions.contentSpacing.x
            onHeightChanged: d.resize(); onWidthChanged: d.resize();
            layoutDirection: Qt.RightToLeft
            anchors {
                bottom: parent.bottom
                right: parent.right
                margins: 0
                bottomMargin: hifi.dimensions.contentSpacing.y
            }
            Button {
                id: cancelButton
                action: cancelAction
                KeyNavigation.left: acceptButton
                KeyNavigation.up: items ? comboBox : textResult
                KeyNavigation.backtab: acceptButton
            }
            Button {
                id: acceptButton
                action: acceptAction
                KeyNavigation.right: cancelButton
                KeyNavigation.up: items ? comboBox : textResult
                KeyNavigation.tab: cancelButton
                KeyNavigation.backtab: items ? comboBox : textResult
            }
        }

        Action {
            id: cancelAction
            text: qsTr("Cancel");
            shortcut: "Esc"
            onTriggered: {
                root.canceled();
                root.destroy();
            }
        }

        Action {
            id: acceptAction
            text: qsTr("OK");
            shortcut: "Return"
            onTriggered: {
                root.result = items ? comboBox.currentText : textResult.text
                root.selected(root.result);
                root.destroy();
            }
        }
    }

    Keys.onPressed: {
        if (!visible) {
            return
        }

        switch (event.key) {
        case Qt.Key_Escape:
        case Qt.Key_Back:
            cancelAction.trigger()
            event.accepted = true;
            break;

        case Qt.Key_Return:
        case Qt.Key_Enter:
            if (acceptButton.focus) {
                acceptAction.trigger()
            } else if (cancelButton.focus) {
                cancelAction.trigger()
            } else if (comboBox.focus || comboBox.popup.focus) {
                comboBox.showList()
            }
            event.accepted = true;
            break;
        }
    }

    Component.onCompleted: {
        keyboardEnabled = HMD.active;
        updateIcon();
        d.resize();
        if (items) {
            comboBox.forceActiveFocus()
        } else {
            textResult.forceActiveFocus()
        }
    }
}
