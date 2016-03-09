//
//  Desktop.qml
//
//  Created by Bradley Austin Davis on 25 Apr 2015
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import QtQuick 2.5
import QtQuick.Controls 1.4
import QtQuick.Dialogs 1.2 as OriginalDialogs

import "../controls-uit"
import "../styles-uit"
import "../windows-uit"

import "messageDialog"

ModalWindow {
    id: root
    HifiConstants { id: hifi }
    implicitWidth: 640
    implicitHeight: 320
    destroyOnCloseButton: true
    destroyOnInvisible: true
    visible: true

    signal selected(int button);

    function click(button) {
        clickedButton = button;
        selected(button);
        destroy();
    }

    function exec() {
        return OffscreenUi.waitForMessageBoxResult(root);
    }

    property alias detailedText: detailedText.text
    property alias text: mainTextContainer.text
    property alias informativeText: informativeTextContainer.text
    onIconChanged: updateIcon();
    property int buttons: OriginalDialogs.StandardButton.Ok
    property int icon: OriginalDialogs.StandardIcon.NoIcon
    property string iconText: ""
    property int defaultButton: OriginalDialogs.StandardButton.NoButton;
    property int clickedButton: OriginalDialogs.StandardButton.NoButton;
    focus: defaultButton === OriginalDialogs.StandardButton.NoButton

    function updateIcon() {
        if (!root) {
            return;
        }
        switch (root.icon) {
            case OriginalDialogs.StandardIcon.Information:
                iconText = "\uF05A";
                break;
            case OriginalDialogs.StandardIcon.Question:
                iconText = "\uF059"
                break;
            case OriginalDialogs.StandardIcon.Warning:
                iconText = "\uF071"
                break;
            case OriginalDialogs.StandardIcon.Critical:
                iconText = "\uF057"
                break;
            default:
                iconText = ""
        }
    }

    Item {
        id: messageBox
        clip: true
        width: pane.width
        height: pane.height

        QtObject {
            id: d
            readonly property real spacing: hifi.dimensions.contentSpacing.x
            readonly property real outerSpacing: hifi.dimensions.contentSpacing.y
            readonly property int minWidth: 480
            readonly property int maxWdith: 1280
            readonly property int minHeight: 120
            readonly property int maxHeight: 720

            function resize() {
                var targetWidth = mainTextContainer.width
                var targetHeight = mainTextContainer.height + 3 * hifi.dimensions.contentSpacing.y
                        + (informativeTextContainer.text != "" ? informativeTextContainer.contentHeight + 3 * hifi.dimensions.contentSpacing.y : 0)
                        + buttons.height
                        + (content.state === "expanded" ? details.implicitHeight + hifi.dimensions.contentSpacing.y : 0)
                root.width = (targetWidth < d.minWidth) ? d.minWidth : ((targetWidth > d.maxWdith) ? d.maxWidth : targetWidth)
                root.height = (targetHeight < d.minHeight) ? d.minHeight: ((targetHeight > d.maxHeight) ? d.maxHeight : targetHeight)
            }
        }

        RalewaySemiBold {
            id: mainTextContainer
            onHeightChanged: d.resize(); onWidthChanged: d.resize();
            wrapMode: Text.WordWrap
            size: hifi.fontSizes.menuItem
            color: hifi.colors.baseGrayHighlight
            anchors {
                top: parent.top
                horizontalCenter: parent.horizontalCenter
                margins: 0
                topMargin: hifi.dimensions.contentSpacing.y
            }
            lineHeight: 2
            lineHeightMode: Text.ProportionalHeight
        }

        RalewaySemiBold {
            id: informativeTextContainer
            onHeightChanged: d.resize(); onWidthChanged: d.resize();
            wrapMode: Text.WordWrap
            size: hifi.fontSizes.menuItem
            color: hifi.colors.baseGrayHighlight
            anchors {
                top: mainTextContainer.bottom
                left: parent.left
                right: parent.right
                margins: 0
                topMargin: text != "" ? hifi.dimensions.contentSpacing.y : 0
            }
        }

        Flow {
            id: buttons
            focus: true
            spacing: d.spacing
            onHeightChanged: d.resize(); onWidthChanged: d.resize();
            layoutDirection: Qt.RightToLeft
            anchors {
                top: informativeTextContainer.text == "" ? mainTextContainer.bottom : informativeTextContainer.bottom
                horizontalCenter: parent.horizontalCenter
                margins: 0
                topMargin: 2 * hifi.dimensions.contentSpacing.y
            }
            MessageDialogButton { dialog: root; text: qsTr("Close"); button: OriginalDialogs.StandardButton.Close; }
            MessageDialogButton { dialog: root; text: qsTr("Abort"); button: OriginalDialogs.StandardButton.Abort; }
            MessageDialogButton { dialog: root; text: qsTr("Cancel"); button: OriginalDialogs.StandardButton.Cancel; }
            MessageDialogButton { dialog: root; text: qsTr("Restore Defaults"); button: OriginalDialogs.StandardButton.RestoreDefaults; }
            MessageDialogButton { dialog: root; text: qsTr("Reset"); button: OriginalDialogs.StandardButton.Reset; }
            MessageDialogButton { dialog: root; text: qsTr("Discard"); button: OriginalDialogs.StandardButton.Discard; }
            MessageDialogButton { dialog: root; text: qsTr("No to All"); button: OriginalDialogs.StandardButton.NoToAll; }
            MessageDialogButton { dialog: root; text: qsTr("No"); button: OriginalDialogs.StandardButton.No; }
            MessageDialogButton { dialog: root; text: qsTr("Yes to All"); button: OriginalDialogs.StandardButton.YesToAll; }
            MessageDialogButton { dialog: root; text: qsTr("Yes"); button: OriginalDialogs.StandardButton.Yes; }
            MessageDialogButton { dialog: root; text: qsTr("Apply"); button: OriginalDialogs.StandardButton.Apply; }
            MessageDialogButton { dialog: root; text: qsTr("Ignore"); button: OriginalDialogs.StandardButton.Ignore; }
            MessageDialogButton { dialog: root; text: qsTr("Retry"); button: OriginalDialogs.StandardButton.Retry; }
            MessageDialogButton { dialog: root; text: qsTr("Save All"); button: OriginalDialogs.StandardButton.SaveAll; }
            MessageDialogButton { dialog: root; text: qsTr("Save"); button: OriginalDialogs.StandardButton.Save; }
            MessageDialogButton { dialog: root; text: qsTr("Open"); button: OriginalDialogs.StandardButton.Open; }
            MessageDialogButton { dialog: root; text: qsTr("OK"); button: OriginalDialogs.StandardButton.Ok; }

            Button {
                id: moreButton
                text: qsTr("Show Details...")
                width: 160
                onClicked: { content.state = (content.state === "" ? "expanded" : "") }
                visible: detailedText && detailedText.length > 0
            }
            MessageDialogButton { dialog: root; text: qsTr("Help"); button: OriginalDialogs.StandardButton.Help; }
        }

        Item {
            id: details
            width: parent.width
            implicitHeight: detailedText.implicitHeight
            height: 0
            clip: true
            anchors {
                top: buttons.bottom
                left: parent.left;
                right: parent.right;
                margins: 0
                topMargin: hifi.dimensions.contentSpacing.y
            }
            Flickable {
                id: flickable
                contentHeight: detailedText.height
                anchors.fill: parent
                anchors.topMargin: root.spacing
                anchors.bottomMargin: root.outerSpacing
                TextEdit {
                    id: detailedText
                    size: hifi.fontSizes.menuItem
                    color: hifi.colors.baseGrayHighlight
                    width: details.width
                    wrapMode: Text.WordWrap
                    readOnly: true
                    selectByMouse: true
                    anchors.margins: 0
                }
            }
        }

        states: [
            State {
                name: "expanded"
                PropertyChanges { target: root; anchors.fill: undefined }
                PropertyChanges { target: details; height: 120 }
                PropertyChanges { target: moreButton; text: qsTr("Hide Details") }
            }
        ]

        Component.onCompleted: updateIcon();
        onStateChanged: d.resize()
    }

    Keys.onPressed: {
        if (!visible) {
            return
        }

        if (event.modifiers === Qt.ControlModifier)
            switch (event.key) {
            case Qt.Key_A:
                event.accepted = true
                detailedText.selectAll()
                break
            case Qt.Key_C:
                event.accepted = true
                detailedText.copy()
                break
            case Qt.Key_Period:
                if (Qt.platform.os === "osx") {
                    event.accepted = true
                    content.reject()
                }
                break
        } else switch (event.key) {
            case Qt.Key_Escape:
            case Qt.Key_Back:
                event.accepted = true
                root.click(OriginalDialogs.StandardButton.Cancel)
                break

            case Qt.Key_Enter:
            case Qt.Key_Return:
                event.accepted = true
                root.click(root.defaultButton)
                break
        }
    }
}
