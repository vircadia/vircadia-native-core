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

import controlsUit 1.0 as HifiControlsUit
import stylesUit 1.0 as HifiStylesUit

import TabletScriptingInterface 1.0

Item {
    id: linkAccountBody
    clip: true
    height: root.pane.height
    width: root.pane.width
    property bool isTablet: root.isTablet
    property bool failAfterSignUp: false

    property bool keyboardEnabled: false
    property bool keyboardRaised: false
    property bool punctuationMode: false

    property bool isLogIn: false
    property bool withSteam: false

    onKeyboardRaisedChanged: d.resize();

    QtObject {
        id: d
        readonly property int minWidth: 480
        readonly property int minWidthButton: !root.isTablet ? 256 : 174
        property int maxWidth: root.isTablet ? 1280 : Window.innerWidth
        readonly property int minHeight: 120
        readonly property int minHeightButton: !root.isTablet ? 56 : 42
        property int maxHeight: root.isTablet ? 720 : Window.innerHeight

        function resize() {
            maxWidth = root.isTablet ? 1280 : Window.innerWidth;
            maxHeight = root.isTablet ? 720 : Window.innerHeight;
            var targetWidth = Math.max(Math.max(titleWidth, topContainer.width), bottomContainer.width);
            var targetHeight =  hifi.dimensions.contentSpacing.y + topContainer.height + bottomContainer.height +
                    4 * hifi.dimensions.contentSpacing.y;

            var newWidth = Math.max(d.minWidth, Math.min(d.maxWidth, targetWidth));
            if(!isNaN(newWidth)) {
                parent.width = root.width = newWidth;
            }

            parent.height = root.height = Math.max(d.minHeight, Math.min(d.maxHeight, targetHeight))
                    + (keyboardEnabled && keyboardRaised ? (200 + 2 * hifi.dimensions.contentSpacing.y) : hifi.dimensions.contentSpacing.y);
        }
    }

    function login() {
        if (linkAccountBody.isLogIn) {
            loginDialog.login(emailField.text, passwordField.text);
        } else {
            loginDialog.signup(emailField.text, usernameField.text, passwordField.text);
        }
        linkAccountBody.toggleLoggingIn(false);
    }

    function toggleLoggingIn(loggedIn) {
        // For the process of logging in.
        if (linkAccountBody.withSteam) {
            loginContainer.visible = false;
        }
        else {

        }
    }

    function toggleSignIn(signIn, isLogIn) {
        // going to/from sign in/up dialog.
        linkAccountBody.isLogIn = isLogIn;
        if (signIn) {
            usernameField.visible = !isLogIn;
            cantAccessContainer.visible = isLogIn;
            if (isLogIn) {
                loginButtonAtSignIn.text = "Log In";
                loginButtonAtSignIn.color = hifi.buttons.black;
                emailField.placeholderText = "Username or Email";
                emailField.anchors.top = loginContainer.top;
                emailField.anchors.topMargin = !root.isTablet ? 0.2 * root.pane.height : 0.24 * root.pane.height;
                cantAccessContainer.anchors.topMargin = !root.isTablet ? 3.5 * hifi.dimensions.contentSpacing.y : hifi.dimensions.contentSpacing.y;
            } else {
                loginButtonAtSignIn.text = "Sign Up";
                loginButtonAtSignIn.color = hifi.buttons.blue;
                emailField.placeholderText = "Email";
                emailField.anchors.top = usernameField.bottom;
                emailField.anchors.topMargin = 1.5 * hifi.dimensions.contentSpacing.y;
            }
        }

        splashContainer.visible = !signIn;
        topContainer.height = signIn ? root.pane.height : 0.6 * topContainer.height;
        bottomContainer.visible = !signIn;
        dismissTextContainer.visible = !signIn;
        topOpaqueRect.visible = signIn;
        loginContainer.visible = signIn;

    }

    function toggleLoading(isLoading) {
        linkAccountSpinner.visible = isLoading
        form.visible = !isLoading

        if (loginDialog.isSteamRunning()) {
            additionalInformation.visible = !isLoading
        }
    }

    Item {
        id: topContainer
        width: root.pane.width
        height: 0.6 * root.pane.height
        onHeightChanged: d.resize(); onWidthChanged: d.resize();

        Rectangle {
            id: topOpaqueRect
            height: parent.height
            width: parent.width
            opacity: 0.9
            color: "black"
            visible: false
        }

        Item {
            id: bannerContainer
            width: parent.width
            height: banner.height
            anchors {
                top: parent.top
                topMargin: 85
            }
            Image {
                id: banner
                anchors.centerIn: parent
                source: "../../images/high-fidelity-banner.svg"
                horizontalAlignment: Image.AlignHCenter
            }
        }
        Item {
            id: loginContainer
            width: parent.width
            height: parent.height - (bannerContainer.height + 1.5 * hifi.dimensions.contentSpacing.y)
            anchors {
                top: bannerContainer.bottom
                topMargin: 1.5 * hifi.dimensions.contentSpacing.y
            }
            visible: false

            HifiControlsUit.TextField {
                id: usernameField
                width: banner.width
                font.family: "Cairo"
                placeholderText: "Username"
                anchors {
                    top: parent.top
                    topMargin: 0.2 * parent.height
                    left: parent.left
                    leftMargin: (parent.width - usernameField.width) / 2
                }
                visible: false
            }

            HifiControlsUit.TextField {
                id: emailField
                width: banner.width
                font.family: "Cairo"
                text: Settings.getValue("wallet/savedUsername", "");
                anchors {
                    top: parent.top
                    left: parent.left
                    leftMargin: (parent.width - emailField.width) / 2
                }
                focus: true
                placeholderText: "Username or Email"
                activeFocusOnPress: true
                onHeightChanged: d.resize(); onWidthChanged: d.resize();

                Component.onCompleted: {
                    var savedUsername = Settings.getValue("wallet/savedUsername", "");
                    emailField.text = savedUsername === "Unknown user" ? "" : savedUsername;
                }
            }
            HifiControlsUit.TextField {
                id: passwordField
                width: banner.width
                font.family: "Cairo"
                placeholderText: "Password"
                activeFocusOnPress: true
                echoMode: passwordFieldMouseArea.showPassword ? TextInput.Normal : TextInput.Password
                anchors {
                    top: emailField.bottom
                    topMargin: 1.5 * hifi.dimensions.contentSpacing.y
                    left: parent.left
                    leftMargin: (parent.width - emailField.width) / 2
                }

                onFocusChanged: {
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
                        width: passwordField.height * 16 / 23
                        height: passwordField.height * 16 / 23
                        anchors {
                            right: parent.right
                            rightMargin: 8
                            top: parent.top
                            topMargin: passwordFieldMouseArea.showPassword ? 6 : 8
                            bottom: parent.bottom
                            bottomMargin: passwordFieldMouseArea.showPassword ? 5 : 8
                        }
                        source: passwordFieldMouseArea.showPassword ?  "../../images/eyeClosed.svg" : "../../images/eyeOpen.svg"
                        MouseArea {
                            id: passwordFieldMouseArea
                            anchors.fill: parent
                            acceptedButtons: Qt.LeftButton
                            property bool showPassword: false
                            onClicked: {
                                showPassword = !showPassword;
                            }
                        }
                    }
                }
                Keys.onReturnPressed: {
                    linkAccountBody.login()
                }
            }
            HifiControlsUit.CheckBox {
                id: autoLogoutCheckbox
                checked: !Settings.getValue("wallet/autoLogout", false);
                text: qsTr("Keep Me Logged In")
                boxSize: 18;
                labelFontFamily: "Cairo"
                labelFontSize: 18;
                color: hifi.colors.white;
                anchors {
                    top: passwordField.bottom;
                    topMargin: hifi.dimensions.contentSpacing.y;
                    right: passwordField.right;
                }
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
            Item {
                id: cancelContainer
                width: cancelText.width
                height: d.minHeightButton
                anchors {
                    top: autoLogoutCheckbox.bottom
                    topMargin: hifi.dimensions.contentSpacing.y
                    left: parent.left
                    leftMargin: (parent.width - passwordField.width) / 2
                }
                Text {
                    id: cancelText
                    anchors.centerIn: parent
                    text: qsTr("Cancel");

                    lineHeight: 1
                    color: "white"
                    font.family: "Cairo"
                    font.pixelSize: 24
                    font.capitalization: Font.AllUppercase;
                    font.bold: true
                    lineHeightMode: Text.ProportionalHeight
                }
                MouseArea {
                    id: cancelArea
                    anchors.fill: parent
                    acceptedButtons: Qt.LeftButton
                    onClicked: {
                        toggleSignIn(false, true);
                    }
                }
            }
            HifiControlsUit.Button {
                id: loginButtonAtSignIn
                width: d.minWidthButton
                height: d.minHeightButton
                text: qsTr("Log In")
                fontFamily: "Cairo"
                fontSize: signUpButton.fontSize
                fontBold: true
                anchors {
                    top: cancelContainer.top
                    right: passwordField.right
                }

                onClicked: {
                    linkAccountBody.login()
                }
            }
            Item {
                id: cantAccessContainer
                width: parent.width
                y: usernameField.height
                anchors {
                    top: cancelContainer.bottom
                    topMargin: 3.5 * hifi.dimensions.contentSpacing.y
                }
                visible: false
                HifiStylesUit.ShortcutText {
                    id: cantAccessText
                    z: 10
                    anchors.centerIn: parent
                    font.family: "Cairo"
                    font.pixelSize: 14

                    text: "<a href='https://highfidelity.com/users/password/new'> Can't access your account?</a>"

                    verticalAlignment: Text.AlignVCenter
                    horizontalAlignment: Text.AlignHCenter
                    linkColor: hifi.colors.blueAccent
                    onLinkActivated: loginDialog.openUrl(link)
                }
            }

        }
        Item {
            id: splashContainer
            width: parent.width
            anchors.fill: parent

            visible: true

            Text {
                id: flavorText
                text: qsTr("BE ANYWHERE, WITH ANYONE RIGHT NOW")
                width: 0.48 * parent.width
                anchors.centerIn: parent
                anchors {
                    top: banner.bottom
                    topMargin: 0.1 * parent.height
                }
                wrapMode: Text.WordWrap
                lineHeight: 1
                color: "white"
                font.family: "Cairo"
                font.pixelSize: 48
                font.bold: true
                lineHeightMode: Text.ProportionalHeight
                horizontalAlignment: Text.AlignHCenter
            }

            HifiControlsUit.Button {
                id: signUpButton
                text: qsTr("Sign Up")
                width: d.minWidthButton
                height: d.minHeightButton
                color: hifi.buttons.blue
                fontFamily: "Cairo"
                fontSize: 21
                fontBold: true
                anchors {
                    bottom: parent.bottom
                    bottomMargin: 0.1 * parent.height
                    left: parent.left
                    leftMargin: (parent.width - signUpButton.width) / 2
                }

                onClicked: {
                    toggleSignIn(true, false);
                }
            }
        }
    }
    Item {
        id: bottomContainer
        width: root.pane.width
        height: 0.4 * root.pane.height
        onHeightChanged: d.resize(); onWidthChanged: d.resize();
        anchors.top: topContainer.bottom

        Rectangle {
            id: bottomOpaqueRect
            height: parent.height
            width: parent.width
            opacity: 0.9
            color: "black"
        }
        Item {
            id: bottomButtonsContainer

            width: parent.width
            height: parent.height

            HifiControlsUit.Button {
                id: loginButton
                width: signUpButton.width
                height: signUpButton.height
                text: qsTr("Log In")
                fontFamily: "Cairo"
                fontSize: signUpButton.fontSize
                fontBold: true
                anchors {
                    top: parent.top
                    topMargin: 0.245 * parent.height
                    left: parent.left
                    leftMargin: (parent.width - loginButton.width) / 2
                }

                onClicked: {
                    toggleSignIn(true, true);
                }
            }
            Button {
                id: steamLoginButton;
                width: signUpButton.width
                height: signUpButton.height
                property int color: hifi.buttons.black;
                property int colorScheme: hifi.colorSchemes.light;

                anchors {
                    top: loginButton.bottom
                    topMargin: 0.04 * parent.height
                    left: parent.left
                    leftMargin: (parent.width - steamLoginButton.width) / 2
                }

                onHoveredChanged: {
                    if (hovered) {
                        Tablet.playSound(TabletEnums.ButtonHover);
                    }
                }

                onFocusChanged: {
                    if (focus) {
                        Tablet.playSound(TabletEnums.ButtonHover);
                    }
                }

                onClicked: {
                    Tablet.playSound(TabletEnums.ButtonClick);
                }

                style: OriginalStyles.ButtonStyle {
                    background: Rectangle {
                        radius: 4;
                        gradient: Gradient {
                            GradientStop {
                                position: 0.2
                                color: {
                                    if (!control.enabled) {
                                        hifi.buttons.disabledColorStart[control.colorScheme]
                                    } else if (control.pressed) {
                                        hifi.buttons.pressedColor[control.color]
                                    } else if (control.hovered) {
                                        hifi.buttons.hoveredColor[control.color]
                                    } else {
                                        hifi.buttons.colorStart[control.color]
                                    }
                                }
                            }
                            GradientStop {
                                position: 1.0
                                color: {
                                    if (!control.enabled) {
                                        hifi.buttons.disabledColorFinish[control.colorScheme]
                                    } else if (control.pressed) {
                                        hifi.buttons.pressedColor[control.color]
                                    } else if (control.hovered) {
                                        hifi.buttons.hoveredColor[control.color]
                                    } else {
                                        hifi.buttons.colorFinish[control.color]
                                    }
                                }
                            }
                        }
                    }

                    label: Item {
                        Image {
                            id: steamIcon;
                            anchors.right: steamIconLabel.left;
                            anchors.rightMargin: 5;
                            anchors.verticalCenter: parent.verticalCenter;
                            source: "../../images/steam.svg";
                            horizontalAlignment: Image.AlignHCenter;
                            sourceSize.width: signUpButton.fontSize + 3
                            sourceSize.height: signUpButton.fontSize + 3
                        }
                        TextMetrics {
                            id: steamIconLabelTextMetrics;
                            font: steamIconLabel.font;
                            text: steamIconLabel.text;
                        }
                        Text {
                            id: steamIconLabel;
                            text: "Steam Log In"
                            anchors.verticalCenter: parent.verticalCenter;
                            width: steamIconLabelTextMetrics.width;
                            x: parent.width/2 - steamIconLabelTextMetrics.width/2 + steamIcon.width/2;
                            font.capitalization: Font.AllUppercase;
                            verticalAlignment: Text.AlignVCenter;
                            horizontalAlignment: Text.AlignHCenter;
                            font.family: "Cairo"
                            font.pixelSize: signUpButton.fontSize;
                            font.bold: true
                            color: enabled ? hifi.buttons.textColor[control.color]
                                            : hifi.buttons.disabledTextColor[control.colorScheme]
                        }
                    }
                }
            }
        }
        Item {
            id: dismissTextContainer
            width: dismissText.width
            height: dismissText.height
            anchors {
                bottom: parent.bottom
                right: parent.right
                margins: 10
            }
            Text {
                id: dismissText
                text: qsTr("No thanks, take me in-world! >")

                lineHeight: 1
                color: "white"
                font.family: "Cairo"
                font.pixelSize: 24
                font.bold: true
                lineHeightMode: Text.ProportionalHeight
                horizontalAlignment: Text.AlignHCenter
            }
            MouseArea {
                id: dismissMouseArea
                anchors.fill: parent
                acceptedButtons: Qt.LeftButton
                onClicked: {
                    root.tryDestroy();
                }
            }
        }
    }

    Component.onCompleted: {
        //dont rise local keyboard
        keyboardEnabled = !root.isTablet && HMD.active;
        //but rise Tablet's one instead for Tablet interface
        if (root.isTablet) {
            root.keyboardEnabled = HMD.active;
            root.keyboardRaised = Qt.binding( function() { return keyboardRaised; })
        }
        d.resize();

    }

    Connections {
        target: loginDialog
        onHandleLoginCompleted: {
            console.log("Login Succeeded")
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
                // TODO you are now logged in
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
            toggleLoading(false)
        }
        onHandleLinkCompleted: {
            console.log("Link Succeeded")

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
            return;
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
