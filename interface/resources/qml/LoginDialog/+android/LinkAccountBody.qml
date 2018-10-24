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

import "../../controls-uit"
import "../../styles-uit"

Item {
    id: linkAccountBody

    clip: true
    height: 300
    width: root.pane.width
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
        readonly property int minWidth: 1440
        readonly property int maxWidth: 3840
        readonly property int minHeight: 150
        readonly property int maxHeight: 660

        function resize() {
            var targetWidth = Math.max(titleWidth, form.contentWidth);
            var targetHeight =  hifi.dimensions.contentSpacing.y + mainTextContainer.height +
                            4 * hifi.dimensions.contentSpacing.y + form.height +
                                hifi.dimensions.contentSpacing.y + buttons.height;

            if (additionalInformation.visible) {
                targetWidth = Math.max(targetWidth, additionalInformation.width);
                targetHeight += hifi.dimensions.contentSpacing.y + additionalInformation.height
            }

            parent.width = root.width = Math.max(d.minWidth, Math.min(d.maxWidth, targetWidth));
            parent.height = 420;
            /*root.height = Math.max(d.minHeight, Math.min(d.maxHeight, targetHeight))
                    + (keyboardEnabled && keyboardRaised ? (200 + 2 * hifi.dimensions.contentSpacing.y) : hifi.dimensions.contentSpacing.y);*/
        }
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

        width: 144
        height: 144
    }

    ShortcutText {
        id: mainTextContainer
        anchors {
            top: parent.top
            left: parent.left
            margins: 0
            topMargin: hifi.dimensions.contentSpacing.y / 2
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
            topMargin: 0 //  hifi.dimensions.contentSpacing.y
        }
        spacing: hifi.dimensions.contentSpacing.y / 2

        TextField {
            id: usernameField
            anchors {
                  horizontalCenter: parent.horizontalCenter
            }
            width: 1080
            placeholderText: qsTr("Username or Email")
        }

        TextField {
            id: passwordField
            anchors {
                  horizontalCenter: parent.horizontalCenter
            }
            width: 1080

            placeholderText: qsTr("Password")
            echoMode: TextInput.Password

            Keys.onReturnPressed: linkAccountBody.login()
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
        lineHeight: 3
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
            top: form.bottom
            topMargin: hifi.dimensions.contentSpacing.y / 2
        }

        spacing: hifi.dimensions.contentSpacing.x
        onHeightChanged: d.resize(); onWidthChanged: d.resize();

        Button {
            anchors.verticalCenter: parent.verticalCenter

            text: qsTr("Sign Up")
            visible: !loginDialog.isSteamRunning()

            onClicked: {
                bodyLoader.setSource("SignUpBody.qml")
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

            text: qsTr(loginDialog.isSteamRunning() ? "Link Account" : "Login")
            color: hifi.buttons.blue

            onClicked: {
                Qt.inputMethod.hide();
                linkAccountBody.login();
            }
        }

        Button {
            anchors.verticalCenter: parent.verticalCenter

            text: qsTr("Cancel")

            onClicked: {
                Qt.inputMethod.hide();
                root.tryDestroy();
            }
        }
    }

    Component.onCompleted: {
        root.title = qsTr("Sign Into High Fidelity")
        root.iconText = "<"
        keyboardEnabled = HMD.active;
        d.resize();

        if (failAfterSignUp) {
            mainTextContainer.text = "Account created successfully."
            mainTextContainer.visible = true
        }

        //usernameField.forceActiveFocus();
    }

    Connections {
        target: loginDialog
        onHandleLoginCompleted: {
            console.log("Login Succeeded, linking steam account")

            if (loginDialog.isSteamRunning()) {
                loginDialog.linkSteam()
            } else {
                bodyLoader.setSource("../WelcomeBody.qml", { "welcomeBack" : true })
                bodyLoader.item.width = root.pane.width
                bodyLoader.item.height = root.pane.height
            }
        }
        onHandleLoginFailed: {
            console.log("Login Failed")
            mainTextContainer.visible = true
            toggleLoading(false)
        }
        onHandleLinkCompleted: {
            console.log("Link Succeeded")

            bodyLoader.setSource("../WelcomeBody.qml", { "welcomeBack" : true })
            bodyLoader.item.width = root.pane.width
            bodyLoader.item.height = root.pane.height
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
