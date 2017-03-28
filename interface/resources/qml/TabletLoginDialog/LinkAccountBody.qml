//
//  LinkAccountBody.qml
//
//  Created by Clement on 7/18/16
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import Hifi 1.0
import QtQuick 2.4
import QtQuick.Controls 1.4
import QtQuick.Controls.Styles 1.4 as OriginalStyles

import "../controls-uit"
import "../styles-uit"

Item {
    id: linkAccountBody
    clip: true
    height: parent.height
    width: parent.width
    property bool failAfterSignUp: false

    function login() {
        mainTextContainer.visible = false
        toggleLoading(true)
        loginDialog.login(usernameField.text, passwordField.text)
    }

    property bool keyboardEnabled: false
    property bool keyboardRaised: false
    property bool punctuationMode: false

    onKeyboardRaisedChanged: d.resize();

    QtObject {
        id: d
        function resize() {}
    }

    function toggleLoading(isLoading) {
        linkAccountSpinner.visible = isLoading
        form.visible = !isLoading

        if (loginDialog.isSteamRunning()) {
            additionalInformation.visible = !isLoading
        }

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

                label: "Username or Email"
            }

            ShortcutText {
                anchors {
                    verticalCenter: parent.verticalCenter
                }

                text: "<a href='https://highfidelity.com/users/password/new'>Forgot Username?</a>"

                verticalAlignment: Text.AlignVCenter
                horizontalAlignment: Text.AlignHCenter
                linkColor: hifi.colors.blueAccent

                onLinkActivated: loginDialog.openUrl(link)
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

            ShortcutText {
                anchors {
                    verticalCenter: parent.verticalCenter
                }

                text: "<a href='https://highfidelity.com/users/password/new'>Forgot Password?</a>"

                verticalAlignment: Text.AlignVCenter
                horizontalAlignment: Text.AlignHCenter
                linkColor: hifi.colors.blueAccent

                onLinkActivated: loginDialog.openUrl(link)
            }
        }

    }

    InfoItem {
        id: additionalInformation
        anchors {
            top: form.bottom
            left: parent.left
            margins: 0
            topMargin: hifi.dimensions.contentSpacing.y
        }

        visible: loginDialog.isSteamRunning()

        text: qsTr("Your steam account informations will not be exposed to other users.")
        wrapMode: Text.WordWrap
        color: hifi.colors.baseGrayHighlight
        lineHeight: 1
        lineHeightMode: Text.ProportionalHeight
        horizontalAlignment: Text.AlignHCenter
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
            bottom: parent.bottom
            bottomMargin: hifi.dimensions.contentSpacing.y
        }

        spacing: hifi.dimensions.contentSpacing.x
        onHeightChanged: d.resize(); onWidthChanged: d.resize();

        Button {
          anchors.verticalCenter: parent.verticalCenter

          text: qsTr("Sign Up")
          visible: !loginDialog.isSteamRunning()

          onClicked: {
              bodyLoader.setSource("SignUpBody.qml")
          }
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

            text: qsTr(loginDialog.isSteamRunning() ? "Link Account" : "Login")
            color: hifi.buttons.blue

            onClicked: linkAccountBody.login()
        }

        Button {
            anchors.verticalCenter: parent.verticalCenter
            text: qsTr("Cancel")
            onClicked: {
                bodyLoader.popup()
            }
        }
    }

    Component.onCompleted: {
        loginDialogRoot.title = qsTr("Sign Into High Fidelity")
        loginDialogRoot.iconText = "<"
        keyboardEnabled = HMD.active;
        d.resize();

        if (failAfterSignUp) {
            mainTextContainer.text = "Account created successfully."
            mainTextContainer.visible = true
        }

        usernameField.forceActiveFocus();
    }

    Connections {
        target: loginDialog
        onHandleLoginCompleted: {
            console.log("Login Succeeded, linking steam account")

            if (loginDialog.isSteamRunning()) {
                loginDialog.linkSteam()
            } else {
                bodyLoader.setSource("WelcomeBody.qml", { "welcomeBack" : true })
            }
        }
        onHandleLoginFailed: {
            console.log("Login Failed")
            mainTextContainer.visible = true
            toggleLoading(false)
        }
        onHandleLinkCompleted: {
            console.log("Link Succeeded")

            bodyLoader.setSource("WelcomeBody.qml", { "welcomeBack" : true })
        }
        onHandleLinkFailed: {
            console.log("Link Failed")
            toggleLoading(false)
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
                linkAccountBody.login()
                break
        }
    }
}
