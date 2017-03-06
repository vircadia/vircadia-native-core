//
//  MessageDialog.qml
//
//  Created by Bradley Austin Davis on 15 Jan 2016
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import QtQuick 2.5
import QtQuick.Controls 1.4
import QtQuick.Dialogs 1.2 as OriginalDialogs

import "../controls-uit"
import "../styles-uit"
import "../windows"

import "messageDialog"

TabletModalWindow {
    id: root
    HifiConstants { id: hifi }
    visible: true

    signal selected(int button);

    MouseArea {
        id: mouse;
        anchors.fill: parent
    }
    
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
    property int buttons: OriginalDialogs.StandardButton.Ok
    property int icon: OriginalDialogs.StandardIcon.NoIcon
    property string iconText: ""
    property int iconSize: 50
    onIconChanged: updateIcon();
    property int defaultButton: OriginalDialogs.StandardButton.NoButton;
    property int clickedButton: OriginalDialogs.StandardButton.NoButton;
    focus: defaultButton === OriginalDialogs.StandardButton.NoButton

    property int titleWidth: 0
    onTitleWidthChanged: d.resize();

    function updateIcon() {
        if (!root) {
            return;
        }
        iconText = hifi.glyphForIcon(root.icon);
    }

    TabletModalFrame {
        id: messageBox
        clip: true
        anchors.verticalCenter: parent.verticalCenter
        anchors.horizontalCenter: parent.horizontalCenter
        width: parent.width - 6
        height: 300

        QtObject {
            id: d
            readonly property int minWidth: 200
            readonly property int maxWidth: 1280
            readonly property int minHeight: 120
            readonly property int maxHeight: 720

            function resize() {
                var targetWidth = Math.max(titleWidth, mainTextContainer.contentWidth)
                var targetHeight = mainTextContainer.height + 3 * hifi.dimensions.contentSpacing.y
                        + (informativeTextContainer.text != "" ? informativeTextContainer.contentHeight + 3 * hifi.dimensions.contentSpacing.y : 0)
                        + buttons.height
                        + (details.implicitHeight + hifi.dimensions.contentSpacing.y) + messageBox.frameMarginTop
                messageBox.height = (targetHeight < d.minHeight) ? d.minHeight: ((targetHeight > d.maxHeight) ? d.maxHeight : targetHeight)
            }
        }

        RalewaySemiBold {
            id: mainTextContainer
            onTextChanged: d.resize();
            wrapMode: Text.WordWrap
            size: hifi.fontSizes.sectionName
            color: hifi.colors.baseGrayHighlight
            width: parent.width - 6
            anchors {
                top: parent.top
                horizontalCenter: parent.horizontalCenter
                margins: 0
                topMargin: hifi.dimensions.contentSpacing.y + messageBox.frameMarginTop
            }
            maximumLineCount: 30
            elide: Text.ElideLeft
            lineHeight: 2
            lineHeightMode: Text.ProportionalHeight
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
        }

        RalewaySemiBold {
            id: informativeTextContainer
            onTextChanged: d.resize();
            wrapMode: Text.WordWrap
            size: hifi.fontSizes.sectionName
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
            spacing: hifi.dimensions.contentSpacing.x
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
                anchors.topMargin: hifi.dimensions.contentSpacing.x
                anchors.bottomMargin: hifi.dimensions.contentSpacing.y
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

        Component.onCompleted: {
            updateIcon();
            d.resize();
        }
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
