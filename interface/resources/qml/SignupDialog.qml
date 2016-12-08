//
//  SignupDialog.qml
//
//  Created by David Rowe on 7 Dec 2016
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import Hifi 1.0
import QtQuick 2.4

import "controls-uit"
import "styles-uit"
import "windows"

import "SignupDialog"

ModalWindow {
    id: root
    HifiConstants { id: hifi }
    objectName: "SignupDialog"
    implicitWidth: 520
    implicitHeight: 320
    destroyOnCloseButton: true
    destroyOnHidden: true
    visible: true

    property string iconText: ""
    property int iconSize: 50

    property string title: ""
    property int titleWidth: 0

    keyboardOverride: true  // Disable ModalWindow's keyboard.

    SignupDialog {
        id: signupDialog

        Item {
            id: signupBody
            clip: true
            width: root.pane.width
            height: root.pane.height

            function signup() {
                mainTextContainer.visible = false
                signupDialog.signup(usernameField.text, passwordField.text)
            }

            property bool keyboardEnabled: false
            property bool keyboardRaised: false
            property bool punctuationMode: false

            onKeyboardRaisedChanged: d.resize();

            QtObject {
                id: d
                readonly property int minWidth: 480
                readonly property int maxWidth: 1280
                readonly property int minHeight: 120
                readonly property int maxHeight: 720

                function resize() {
                    var targetWidth = Math.max(titleWidth, form.contentWidth);
                    var targetHeight =  hifi.dimensions.contentSpacing.y + mainTextContainer.height +
                                    4 * hifi.dimensions.contentSpacing.y + form.height +
                                        hifi.dimensions.contentSpacing.y + buttons.height;

                    if (additionalInformation.visible) {
                        targetWidth = Math.max(targetWidth, additionalInformation.width);
                        targetHeight += hifi.dimensions.contentSpacing.y + additionalInformation.height
                    }

                    root.width = Math.max(d.minWidth, Math.min(d.maxWidth, targetWidth));
                    root.height = Math.max(d.minHeight, Math.min(d.maxHeight, targetHeight))
                            + (keyboardEnabled && keyboardRaised ? (200 + 2 * hifi.dimensions.contentSpacing.y) : hifi.dimensions.contentSpacing.y);
                }
            }

            ShortcutText {
                id: mainTextContainer
                anchors {
                    top: parent.top
                    left: parent.left
                    margins: 0
                    topMargin: hifi.dimensions.contentSpacing.y
                }

                visible: false

                text: qsTr("Username or password incorrect.")
                wrapMode: Text.WordWrap
                color: hifi.colors.redAccent
                lineHeight: 1
                lineHeightMode: Text.ProportionalHeight
                horizontalAlignment: Text.AlignHCenter
            }

            Column {
                id: form
                anchors {
                    top: mainTextContainer.bottom
                    left: parent.left
                    margins: 0
                    topMargin: 2 * hifi.dimensions.contentSpacing.y
                }
                spacing: 2 * hifi.dimensions.contentSpacing.y

                Row {
                    spacing: hifi.dimensions.contentSpacing.x

                    TextField {
                        id: usernameField
                        anchors {
                            verticalCenter: parent.verticalCenter
                        }
                        width: 350

                        label: "User Name or Email"
                    }
                }
                Row {
                    spacing: hifi.dimensions.contentSpacing.x

                    TextField {
                        id: passwordField
                        anchors {
                            verticalCenter: parent.verticalCenter
                        }
                        width: 350

                        label: "Password"
                        echoMode: TextInput.Password
                    }
                }

            }

            // Override ScrollingWindow's keyboard that would be at very bottom of dialog.
            Keyboard {
                raised: keyboardEnabled && keyboardRaised
                numeric: punctuationMode
                anchors {
                    left: parent.left
                    right: parent.right
                    bottom: buttons.top
                    bottomMargin: keyboardRaised ? 2 * hifi.dimensions.contentSpacing.y : 0
                }
            }

            Row {
                id: buttons
                anchors {
                    right: parent.right
                    bottom: parent.bottom
                    bottomMargin: hifi.dimensions.contentSpacing.y
                }
                spacing: hifi.dimensions.contentSpacing.x
                onHeightChanged: d.resize(); onWidthChanged: d.resize();

                Button {
                    id: linkAccountButton
                    anchors.verticalCenter: parent.verticalCenter
                    width: 200

                    text: qsTr("Signup")
                    color: hifi.buttons.blue

                    onClicked: signupBody.signup()
                }

                Button {
                    anchors.verticalCenter: parent.verticalCenter

                    text: qsTr("Cancel")

                    onClicked: root.destroy()
                }
            }

            Component.onCompleted: {
                root.title = qsTr("Sign into High Fidelity")
                root.iconText = "<"
                keyboardEnabled = HMD.active;
                d.resize();

                usernameField.forceActiveFocus();
            }

            Connections {
                target: signupDialog
                onHandleSignupCompleted: {
                    console.log("Signup Succeeded")

                    bodyLoader.setSource("WelcomeBody.qml", { "welcomeBack" : true })
                    bodyLoader.item.width = root.pane.width
                    bodyLoader.item.height = root.pane.height
                }
                onHandleSignupFailed: {
                    console.log("Signup Failed")
                    mainTextContainer.visible = true
                }
            }

            Keys.onPressed: {
                if (!visible) {
                    return
                }

                switch (event.key) {
                    case Qt.Key_Enter:
                    case Qt.Key_Return:
                        event.accepted = true
                        signupBody.signup()
                        break
                }
            }
        }
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
                destroy()
                break

            case Qt.Key_Enter:
            case Qt.Key_Return:
                event.accepted = true
                break
        }
    }
}
