//
//  SignUpBody.qml
//
//  Created by Stephen Birarda on 7 Dec 2016
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import Hifi 1.0
import QtQuick 2.4
import QtQuick.Controls 1.4
import QtQuick.Controls.Styles 1.4 as OriginalStyles

import controlsUit 1.0
import stylesUit 1.0

Item {
    id: signupBody

    clip: true
    height: root.pane.height
    width: root.pane.width

    function signup() {
        mainTextContainer.visible = false
        toggleLoading(true)
        loginDialog.signup(emailField.text, usernameField.text, passwordField.text)
    }

    property bool keyboardEnabled: false
    property bool keyboardRaised: false
    property bool punctuationMode: false

    onKeyboardRaisedChanged: d.resize();

    QtObject {
        id: d
        readonly property int minWidth: 960
        readonly property int maxWidth: 2560
        readonly property int minHeight: 240
        readonly property int maxHeight: 1480

        function resize() {
            var targetWidth = Math.max(titleWidth, form.contentWidth);
            var targetHeight =  hifi.dimensions.contentSpacing.y + mainTextContainer.height +
                                4 * hifi.dimensions.contentSpacing.y + form.height +
                                hifi.dimensions.contentSpacing.y + buttons.height;

            parent.width = root.width = Math.max(d.minWidth, Math.min(d.maxWidth, targetWidth));
            //parent.height = 650;
            parent.height = root.height = Math.max(d.minHeight, Math.min(d.maxHeight, targetHeight));
                
        }
    }

    function toggleLoading(isLoading) {
        linkAccountSpinner.visible = isLoading
        form.visible = !isLoading

        leftButton.visible = !isLoading
        buttons.visible = !isLoading
    }

    BusyIndicator {
        id: linkAccountSpinner

        anchors {
            top: parent.top
            horizontalCenter: parent.horizontalCenter
            topMargin: hifi.dimensions.contentSpacing.y
        }

        visible: false
        running: true

        width: 48
        height: 48
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

        text: qsTr("There was an unknown error while creating your account.")
        wrapMode: Text.WordWrap
        color: hifi.colors.redAccent
        horizontalAlignment: Text.AlignLeft
    }

    Column {
        id: form
        anchors {
            top: mainTextContainer.bottom
            left: parent.left
            margins: 0
            topMargin: 0; // 2 * hifi.dimensions.contentSpacing.y
        }
        spacing: hifi.dimensions.contentSpacing.y / 2

        Row {
            spacing: hifi.dimensions.contentSpacing.x

            TextField {
                id: emailField
                anchors {
                    verticalCenter: parent.verticalCenter
                }
                width: 780

                placeholderText: "Email"
            }
        }

        Row {
            spacing: hifi.dimensions.contentSpacing.x

            TextField {
                id: usernameField
                anchors {
                    verticalCenter: parent.verticalCenter
                }
                width: 780

                placeholderText: "Username"
            }

            ShortcutText {
                anchors {
                    verticalCenter: parent.verticalCenter
                }

                text: qsTr("No spaces / special chars.")

                verticalAlignment: Text.AlignVCenter
                horizontalAlignment: Text.AlignHCenter

                color: hifi.colors.blueAccent
            }
        }

        Row {
            spacing: hifi.dimensions.contentSpacing.x

            TextField {
                id: passwordField
                anchors {
                    verticalCenter: parent.verticalCenter
                }
                width: 780

                placeholderText: "Password"
                echoMode: TextInput.Password
            }

            ShortcutText {
                anchors {
                    verticalCenter: parent.verticalCenter
                }

                text: qsTr("At least 6 characters")

                verticalAlignment: Text.AlignVCenter
                horizontalAlignment: Text.AlignHCenter

                color: hifi.colors.blueAccent
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
        id: leftButton
        anchors {
            left: parent.left
            top: form.bottom
            topMargin: hifi.dimensions.contentSpacing.y// / 2
        }

        spacing: hifi.dimensions.contentSpacing.x
        onHeightChanged: d.resize(); onWidthChanged: d.resize();

        Button {
            anchors.verticalCenter: parent.verticalCenter

            text: qsTr("Existing User")

            onClicked: {
                bodyLoader.setSource("LinkAccountBody.qml")
                bodyLoader.item.width = root.pane.width
                bodyLoader.item.height = root.pane.height
            }
        }
    }

    Row {
        id: buttons
        anchors {
            right: parent.right
            top: form.bottom
            topMargin: hifi.dimensions.contentSpacing.y / 2
        }
        spacing: hifi.dimensions.contentSpacing.x
        onHeightChanged: d.resize(); onWidthChanged: d.resize();

        Button {
            id: linkAccountButton
            anchors.verticalCenter: parent.verticalCenter

            text: qsTr("Sign Up")
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
        root.title = qsTr("Create an Account")
        root.iconText = "<"
        keyboardEnabled = HMD.active;
        d.resize();

        emailField.forceActiveFocus();
    }

    Connections {
        target: loginDialog
        onHandleSignupCompleted: {
            console.log("Sign Up Succeeded");

            // now that we have an account, login with that username and password
            loginDialog.login(usernameField.text, passwordField.text)
        }
        onHandleSignupFailed: {
            console.log("Sign Up Failed")
            toggleLoading(false)

            mainTextContainer.text = errorString
            mainTextContainer.visible = true

            d.resize();
        }
        onHandleLoginCompleted: {
            bodyLoader.setSource("../WelcomeBody.qml", { "welcomeBack": false })
            bodyLoader.item.width = root.pane.width
            bodyLoader.item.height = root.pane.height
        }
        onHandleLoginFailed: {
            // we failed to login, show the LoginDialog so the user will try again
            bodyLoader.setSource("LinkAccountBody.qml", { "failAfterSignUp": true })
            bodyLoader.item.width = root.pane.width
            bodyLoader.item.height = root.pane.height
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
