/*****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtQuick.Dialogs module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of Digia Plc and its Subsidiary(-ies) nor the names
**     of its contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
*****************************************************************************/

import Hifi 1.0 as Hifi
import QtQuick 2.2
import QtQuick.Controls 1.2
import QtQuick.Dialogs 1.2
import "controls"

Dialog {
    id: root
    property real spacing: 8
    property real outerSpacing: 16

    destroyOnCloseButton: true
    destroyOnInvisible: true
    implicitHeight: content.implicitHeight + outerSpacing * 2 + 48
    implicitWidth: Math.min(200, Math.max(mainText.implicitWidth, content.buttonsRowImplicitWidth) + outerSpacing * 2);

    onImplicitHeightChanged: root.height = implicitHeight
    onImplicitWidthChanged: root.width = implicitWidth

    SystemPalette { id: palette }

    function calculateImplicitWidth() {
        if (buttons.visibleChildren.length < 2)
            return;
        var calcWidth = 0;
        for (var i = 0; i < buttons.visibleChildren.length; ++i) {
            calcWidth += Math.max(100, buttons.visibleChildren[i].implicitWidth) + root.spacing
        }
        content.buttonsRowImplicitWidth = outerSpacing + calcWidth + 48
    }
    
    onEnabledChanged: {
        if (enabled) {
        	content.forceActiveFocus();
        }
    }

    Hifi.MessageDialog {
    	id: content
    	clip: true
        anchors.fill: parent
        anchors.topMargin: parent.topMargin + root.outerSpacing
        anchors.leftMargin: parent.margins + root.outerSpacing
        anchors.rightMargin: parent.margins + root.outerSpacing
        anchors.bottomMargin: parent.margins + root.outerSpacing
        implicitHeight: contentColumn.implicitHeight + outerSpacing * 2
        implicitWidth: Math.max(mainText.implicitWidth, buttonsRowImplicitWidth);
        property real buttonsRowImplicitWidth: Screen.pixelDensity * 50

        Keys.onPressed: {
        	console.log("Key press at content")
            event.accepted = true
            if (event.modifiers === Qt.ControlModifier)
                switch (event.key) {
                case Qt.Key_A:
                	console.log("Select All")
                    detailedText.selectAll()
                    break
                case Qt.Key_C:
                	console.log("Copy")
                    detailedText.copy()
                    break
                case Qt.Key_Period:
                    if (Qt.platform.os === "osx")
                        reject()
                    break
            } else switch (event.key) {
                case Qt.Key_Escape:
                case Qt.Key_Back:
                	console.log("Rejecting")
                    reject()
                    break
                case Qt.Key_Enter:
                case Qt.Key_Return:
                	console.log("Accepting")
                    accept()
                    break
            }
        }
        
        onImplicitWidthChanged: root.width = implicitWidth

        Component.onCompleted: {
            root.title = title
        }
        	
        onTitleChanged: {
        	root.title = title
        }
    	
        Column {
            id: contentColumn
            spacing: root.outerSpacing
            anchors {
            	top: parent.top
            	left: parent.left
            	right: parent.right
            }

            Item {
                width: parent.width
                height: Math.max(icon.height, mainText.height + informativeText.height + root.spacing)
                Image {
                    id: icon
                    source: content.standardIconSource
                }

                Text {
                    id: mainText
                    anchors {
                        left: icon.right
                        leftMargin: root.spacing
                        right: parent.right
                    }
                    text: content.text
                    font.pointSize: 14
                    font.weight: Font.Bold
                    wrapMode: Text.WordWrap
                }

                Text {
                    id: informativeText
                    anchors {
                        left: icon.right
                        right: parent.right
                        top: mainText.bottom
                        leftMargin: root.spacing
                        topMargin: root.spacing
                    }
                    text: content.informativeText
                    font.pointSize: 14
                    wrapMode: Text.WordWrap
                }
            }


            Flow {
                id: buttons
                spacing: root.spacing
                layoutDirection: Qt.RightToLeft
                width: parent.width
                Button {
                    id: okButton
                    text: qsTr("OK")
                    onClicked: content.click(StandardButton.Ok)
                    visible: content.standardButtons & StandardButton.Ok
                }
                Button {
                    id: openButton
                    text: qsTr("Open")
                    onClicked: content.click(StandardButton.Open)
                    visible: content.standardButtons & StandardButton.Open
                }
                Button {
                    id: saveButton
                    text: qsTr("Save")
                    onClicked: content.click(StandardButton.Save)
                    visible: content.standardButtons & StandardButton.Save
                }
                Button {
                    id: saveAllButton
                    text: qsTr("Save All")
                    onClicked: content.click(StandardButton.SaveAll)
                    visible: content.standardButtons & StandardButton.SaveAll
                }
                Button {
                    id: retryButton
                    text: qsTr("Retry")
                    onClicked: content.click(StandardButton.Retry)
                    visible: content.standardButtons & StandardButton.Retry
                }
                Button {
                    id: ignoreButton
                    text: qsTr("Ignore")
                    onClicked: content.click(StandardButton.Ignore)
                    visible: content.standardButtons & StandardButton.Ignore
                }
                Button {
                    id: applyButton
                    text: qsTr("Apply")
                    onClicked: content.click(StandardButton.Apply)
                    visible: content.standardButtons & StandardButton.Apply
                }
                Button {
                    id: yesButton
                    text: qsTr("Yes")
                    onClicked: content.click(StandardButton.Yes)
                    visible: content.standardButtons & StandardButton.Yes
                }
                Button {
                    id: yesAllButton
                    text: qsTr("Yes to All")
                    onClicked: content.click(StandardButton.YesToAll)
                    visible: content.standardButtons & StandardButton.YesToAll
                }
                Button {
                    id: noButton
                    text: qsTr("No")
                    onClicked: content.click(StandardButton.No)
                    visible: content.standardButtons & StandardButton.No
                }
                Button {
                    id: noAllButton
                    text: qsTr("No to All")
                    onClicked: content.click(StandardButton.NoToAll)
                    visible: content.standardButtons & StandardButton.NoToAll
                }
                Button {
                    id: discardButton
                    text: qsTr("Discard")
                    onClicked: content.click(StandardButton.Discard)
                    visible: content.standardButtons & StandardButton.Discard
                }
                Button {
                    id: resetButton
                    text: qsTr("Reset")
                    onClicked: content.click(StandardButton.Reset)
                    visible: content.standardButtons & StandardButton.Reset
                }
                Button {
                    id: restoreDefaultsButton
                    text: qsTr("Restore Defaults")
                    onClicked: content.click(StandardButton.RestoreDefaults)
                    visible: content.standardButtons & StandardButton.RestoreDefaults
                }
                Button {
                    id: cancelButton
                    text: qsTr("Cancel")
                    onClicked: content.click(StandardButton.Cancel)
                    visible: content.standardButtons & StandardButton.Cancel
                }
                Button {
                    id: abortButton
                    text: qsTr("Abort")
                    onClicked: content.click(StandardButton.Abort)
                    visible: content.standardButtons & StandardButton.Abort
                }
                Button {
                    id: closeButton
                    text: qsTr("Close")
                    onClicked: content.click(StandardButton.Close)
                    visible: content.standardButtons & StandardButton.Close
                }
                Button {
                    id: moreButton
                    text: qsTr("Show Details...")
                    onClicked: content.state = (content.state === "" ? "expanded" : "")
                    visible: content.detailedText.length > 0
                }
                Button {
                    id: helpButton
                    text: qsTr("Help")
                    onClicked: content.click(StandardButton.Help)
                    visible: content.standardButtons & StandardButton.Help
                }
                onVisibleChildrenChanged: root.calculateImplicitWidth()
            }
        }

        Item {
            id: details
            width: parent.width
            implicitHeight: detailedText.implicitHeight + root.spacing
            height: 0
            clip: true

            anchors {
                left: parent.left
                right: parent.right
                top: contentColumn.bottom
                topMargin: root.spacing
                leftMargin: root.outerSpacing
                rightMargin: root.outerSpacing
            }

            Flickable {
                id: flickable
                contentHeight: detailedText.height
                anchors.fill: parent
                anchors.topMargin: root.spacing
                anchors.bottomMargin: root.outerSpacing
                TextEdit {
                    id: detailedText
                    text: content.detailedText
                    width: details.width
                    wrapMode: Text.WordWrap
                    readOnly: true
                    selectByMouse: true
                }
            }
        }

        states: [
            State {
                name: "expanded"
                PropertyChanges {
                    target: details
                    height: root.height - contentColumn.height - root.spacing - root.outerSpacing
                }
                PropertyChanges {
                    target: content
                    implicitHeight: contentColumn.implicitHeight + root.spacing * 2 +
                        detailedText.implicitHeight + root.outerSpacing * 2
                }
                PropertyChanges {
                    target: moreButton
                    text: qsTr("Hide Details")
                }
            }
        ]
        
/*        
        Rectangle {

        }
        Component.onCompleted: calculateImplicitWidth()
        */
    }
}
