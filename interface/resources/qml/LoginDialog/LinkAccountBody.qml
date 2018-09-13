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
import QtQuick 2.7
import QtQuick.Controls 1.4
import QtQuick.Controls.Styles 1.4 as OriginalStyles

import "../controls-uit"
import "../styles-uit"
Item {
    id: linkAccountBody
    clip: true
    height: root.pane.height
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
        readonly property int minWidth: 480
        readonly property int maxWidth: 1280
        readonly property int minHeight: 120
        readonly property int maxHeight: 720

        function resize() {
            var targetWidth = Math.max(titleWidth, form.contentWidth);
            var targetHeight =  hifi.dimensions.contentSpacing.y + mainTextContainer.height +
                    4 * hifi.dimensions.contentSpacing.y + form.height;

            if (additionalInformation.visible) {
                targetWidth = Math.max(targetWidth, additionalInformation.width);
                targetHeight += hifi.dimensions.contentSpacing.y + additionalInformation.height
            }

            var newWidth = Math.max(d.minWidth, Math.min(d.maxWidth, targetWidth));
            if(!isNaN(newWidth)) {
                parent.width = root.width = newWidth;
            }

            parent.height = root.height = Math.max(d.minHeight, Math.min(d.maxHeight, targetHeight))
                    + (keyboardEnabled && keyboardRaised ? (200 + 2 * hifi.dimensions.contentSpacing.y) : hifi.dimensions.contentSpacing.y);
        }
    }

    function toggleLoading(isLoading) {
        linkAccountSpinner.visible = isLoading
        form.visible = !isLoading

        if (loginDialog.isSteamRunning()) {
            additionalInformation.visible = !isLoading
        }
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
        id: flavorText
        anchors {
            top: parent.top
            left: parent.left
            margins: 0
            topMargin: hifi.dimensions.contentSpacing.y
        }

        text: qsTr("Sign in to High Fidelity to make friends, get HFC, and buy interesting things on the Marketplace!")
        width: parent.width
        wrapMode: Text.WordWrap
        lineHeight: 1
        lineHeightMode: Text.ProportionalHeight
        horizontalAlignment: Text.AlignHCenter
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
        width: parent.width
        onHeightChanged: d.resize(); onWidthChanged: d.resize();

        anchors {
            top: mainTextContainer.bottom
            topMargin: 2 * hifi.dimensions.contentSpacing.y
        }
        spacing: 2 * hifi.dimensions.contentSpacing.y

        TextField {
            id: usernameField
            text: Settings.getValue("wallet/savedUsername", "");
            width: parent.width
            focus: true
            placeholderText: "Username or Email"
            activeFocusOnPress: true

            ShortcutText {
                z: 10
                y: usernameField.height
                anchors {
                    right: usernameField.right
                    top: usernameField.bottom
                    topMargin: 4
                }

                text: "<a href='https://highfidelity.com/users/password/new'>Forgot Username?</a>"

                verticalAlignment: Text.AlignVCenter
                horizontalAlignment: Text.AlignHCenter
                linkColor: hifi.colors.blueAccent

                onLinkActivated: loginDialog.openUrl(link)
            }

            onFocusChanged: {
                root.text = "";
            }
            Component.onCompleted: {
                var savedUsername = Settings.getValue("wallet/savedUsername", "");
                usernameField.text = savedUsername === "Unknown user" ? "" : savedUsername;
            }
        }

        TextField {
            id: passwordField
            width: parent.width
            placeholderText: "Password"
            activeFocusOnPress: true
            echoMode: TextInput.Password
            onHeightChanged: d.resize(); onWidthChanged: d.resize();

            ShortcutText {
                id: forgotPasswordShortcut
                y: passwordField.height
                z: 10
                anchors {
                    right: passwordField.right
                    top: passwordField.bottom
                    topMargin: 4
                }

                text: "<a href='https://highfidelity.com/users/password/new'>Forgot Password?</a>"

                verticalAlignment: Text.AlignVCenter
                horizontalAlignment: Text.AlignHCenter
                linkColor: hifi.colors.blueAccent

                onLinkActivated: loginDialog.openUrl(link)
            }

            onFocusChanged: {
                root.text = "";
                root.isPassword = true;
            }

            Rectangle {
                id: showPasswordHitbox
                z: 10
                x: passwordField.width - ((passwordField.height) * 31 / 23)
                width: parent.width - (parent.width - (parent.height * 31/16))
                height: parent.height
                anchors {
                    right: parent.right
                }
                color: "transparent"

                Image {
                    id: showPasswordImage
                    y: (passwordField.height - (passwordField.height * 16 / 23)) / 2
                    width: passwordField.width - (passwordField.width - (((passwordField.height) * 31/23)))
                    height: passwordField.height * 16 / 23
                    anchors {
                        right: parent.right
                        rightMargin: 3
                    }
                    source: "../../images/eyeOpen.svg"
                }

                MouseArea {
                    id: passwordFieldMouseArea
                    anchors.fill: parent
                    acceptedButtons: Qt.LeftButton
                    property bool showPassword: false
                    onClicked: {
                        showPassword = !showPassword;
                        passwordField.echoMode = showPassword ? TextInput.Normal : TextInput.Password;
                        showPasswordImage.source = showPassword ?  "../../images/eyeClosed.svg" : "../../images/eyeOpen.svg";
                        showPasswordImage.height = showPassword ?  passwordField.height : passwordField.height * 16 / 23;
                        showPasswordImage.y = showPassword ? 0 : (passwordField.height - showPasswordImage.height) / 2;
                    }
                }
            }

            Keys.onReturnPressed: linkAccountBody.login()
        }

        InfoItem {
            id: additionalInformation

            visible: loginDialog.isSteamRunning()

            text: qsTr("Your steam account informations will not be exposed to other users.")
            wrapMode: Text.WordWrap
            color: hifi.colors.baseGrayHighlight
            lineHeight: 1
            lineHeightMode: Text.ProportionalHeight
            horizontalAlignment: Text.AlignHCenter
        }

        Row {
            id: buttons
            spacing: hifi.dimensions.contentSpacing.y*2
            onHeightChanged: d.resize(); onWidthChanged: d.resize();
            anchors.horizontalCenter: parent.horizontalCenter

            CheckBox {
                id: autoLogoutCheckbox
                checked: !Settings.getValue("wallet/autoLogout", true)
                text: "Keep me signed in"
                boxSize: 20;
                labelFontSize: 15
                color: hifi.colors.black
                onCheckedChanged: {
                    Settings.setValue("wallet/autoLogout", !checked);
                    if (checked) {
                        Settings.setValue("wallet/savedUsername", Account.username);
                    } else {
                        Settings.setValue("wallet/savedUsername", "");
                    }
                }
                Component.onDestruction: {
                    Settings.setValue("wallet/autoLogout", !checked);
                }
            }

            Button {
                id: linkAccountButton
                anchors.verticalCenter: parent.verticalCenter
                width: 200

                text: qsTr(loginDialog.isSteamRunning() ? "Link Account" : "Login")
                color: hifi.buttons.blue

                onClicked: linkAccountBody.login()
            }
        }

        Row {
            id: leftButton

            anchors.horizontalCenter: parent.horizontalCenter
            spacing: hifi.dimensions.contentSpacing.y*2
            onHeightChanged: d.resize(); onWidthChanged: d.resize();

            RalewaySemiBold {
                size: hifi.fontSizes.inputLabel
                anchors.verticalCenter: parent.verticalCenter
                text: qsTr("New to High Fidelity?")
            }

            Button {
                anchors.verticalCenter: parent.verticalCenter

                text: qsTr("Sign Up")
                visible: !loginDialog.isSteamRunning()

                onClicked: {
                    bodyLoader.setSource("SignUpBody.qml")
                    if (!root.isTablet) {
                        bodyLoader.item.width = root.pane.width
                        bodyLoader.item.height = root.pane.height
                    }
                }
            }
        }
    }

    Component.onCompleted: {
        root.title = qsTr("Sign Into High Fidelity")
        root.iconText = "<"

        //dont rise local keyboard
        keyboardEnabled = !root.isTablet && HMD.active;
        //but rise Tablet's one instead for Tablet interface
        if (root.isTablet) {
            root.keyboardEnabled = HMD.active;
            root.keyboardRaised = Qt.binding( function() { return keyboardRaised; })
        }
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
            var poppedUp = Settings.getValue("loginDialogPoppedUp", false);
            if (poppedUp) {
                console.log("[ENCOURAGELOGINDIALOG]: logging in")
                var data = {
                    "action": "user logged in"
                };
                UserActivityLogger.logAction("encourageLoginDialog", data);
                Settings.setValue("loginDialogPoppedUp", false);
            }
            if (loginDialog.isSteamRunning()) {
                loginDialog.linkSteam()
            } else {
                bodyLoader.setSource("WelcomeBody.qml", { "welcomeBack" : true })
                bodyLoader.item.width = root.pane.width
                bodyLoader.item.height = root.pane.height
            }
        }
        onHandleLoginFailed: {
            console.log("Login Failed")
            var poppedUp = Settings.getValue("loginDialogPoppedUp", false);
            if (poppedUp) {
                console.log("[ENCOURAGELOGINDIALOG]: failed logging in")
                var data = {
                    "action": "user failed logging in"
                };
                UserActivityLogger.logAction("encourageLoginDialog", data);
                Settings.setValue("loginDialogPoppedUp", false);
            }
            mainTextContainer.visible = true
            toggleLoading(false)
        }
        onHandleLinkCompleted: {
            console.log("Link Succeeded")

            bodyLoader.setSource("WelcomeBody.qml", { "welcomeBack" : true })
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
