//
//  SignInBody.qml
//
//  Created by Wayne Chen on 10/18/18
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import Hifi 1.0
import QtQuick 2.7
import QtQuick.Controls 1.4
import QtQuick.Controls.Styles 1.4 as OriginalStyles

import "qrc:////qml//controls-uit" as HifiControlsUit
import "qrc:////qml//styles-uit" as HifiStylesUit

import TabletScriptingInterface 1.0

Item {
    id: signInBody
    clip: true
    height: root.pane.height
    width: root.pane.width
    property int textFieldHeight: 31
    property bool failAfterSignUp: false
    property string fontFamily: "Cairo"
    property int fontSize: 24
    property bool fontBold: true

    property bool keyboardEnabled: false
    property bool keyboardRaised: false
    property bool punctuationMode: false

    property bool withSteam: loginDialog.isSteamRunning()

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
            var targetWidth = Math.max(titleWidth, mainContainer.width);
            var targetHeight =  hifi.dimensions.contentSpacing.y + mainContainer.height +
                    4 * hifi.dimensions.contentSpacing.y;

            var newWidth = Math.max(d.minWidth, Math.min(d.maxWidth, targetWidth));
            if (!isNaN(newWidth)) {
                parent.width = root.width = newWidth;
            }

            parent.height = root.height = Math.max(d.minHeight, Math.min(d.maxHeight, targetHeight))
                    + (keyboardEnabled && keyboardRaised ? (200 + 2 * hifi.dimensions.contentSpacing.y) : hifi.dimensions.contentSpacing.y);
        }
    }

    // timer to kill the dialog upon login success
    Timer {
        id: successTimer
        interval: 500;
        running: false;
        repeat: false;
        onTriggered: {
            root.tryDestroy();
        }
    }

    // timer to kill the dialog upon login failure
    Timer {
        id: failureTimer
        interval: 1000;
        running: false;
        repeat: false;
        onTriggered: {
            resetContainers();
            loginContainer.visible = true;
        }
    }

    function login() {
        signInBody.toggleLoading();
        if (loginDialog.isLogIn) {
            loginDialog.login(emailField.text, passwordField.text);
        } else {
            loginDialog.signup(usernameField.text, emailField.text, passwordField.text);
        }
    }

    function resetContainers() {
        loggingInContainer.visible = false;
        loginContainer.visible = false;
    }

    function toggleLoading() {
        // For the process of logging in.
        signInBody.resetContainers();
        loggingInContainer.visible = true;
        if (loginDialog.isSteamRunning()) {
            loggingInGlyph.visible = true;
            loggingInText.text = "Logging in to Steam";
            loggingInText.x = loggingInHeader.width/2 - loggingInTextMetrics.width/2 + loggingInGlyphTextMetrics.width/2;
        } else {
            loggingInText.text = "Logging in";
            loggingInText.anchors.bottom = loggingInHeader.bottom;
            loggingInText.anchors.bottomMargin = hifi.dimensions.contentSpacing.y;
        }
        linkAccountSpinner.visible = true;
    }
    function loadingSuccess(success, errorString) {
        linkAccountSpinner.visible = false;
        if (!success) {
            loginErrorMessage.visible = true;
            loginErrorMessage.text = errorString !== "" ? errorString : "unknown error.";
            loginErrorMessage.anchors.bottom = loginDialog.isLogIn ? emailField.top : usernameField.top;
            failureTimer.start();
            return;
        }
        if (loginDialog.isSteamRunning() && !loginDialog.isLogIn) {
            // reset the flag.
            signInBody.withSteam = false;
            loggingInGlyph.visible = false;
            loggingInText.text = "You are now logged into Steam!"
            loggingInText.anchors.centerIn = loggingInHeader;
            loggingInText.anchors.bottom = loggingInHeader.bottom;
            loggedInGlyph.visible = true;
        } else {
            loggingInText.text = "You are now logged in!";
        }
        successTimer.start();
    }

    function init(isLogIn) {
        // going to/from sign in/up dialog.
        loginDialog.isLogIn = isLogIn;
        usernameField.visible = !isLogIn;
        cantAccessContainer.visible = isLogIn;
        if (isLogIn) {
            loginButtonAtSignIn.text = "Log In";
            loginButtonAtSignIn.color = hifi.buttons.black;
            emailField.placeholderText = "Username or Email";
            var savedUsername = Settings.getValue("wallet/savedUsername", "");
            emailField.text = savedUsername === "Unknown user" ? "" : savedUsername;
            emailField.anchors.top = loginContainer.top;
            emailField.anchors.topMargin = !root.isTablet ? 0.2 * root.pane.height : 0.24 * root.pane.height;
            cantAccessContainer.anchors.topMargin = !root.isTablet ? 3.5 * hifi.dimensions.contentSpacing.y : hifi.dimensions.contentSpacing.y;
        } else if (loginDialog.isSteamRunning()) {
            signInBody.toggleLoading();
            loginDialog.loginWithSteam();
        } else {
            loginButtonAtSignIn.text = "Sign Up";
            loginButtonAtSignIn.color = hifi.buttons.blue;
            emailField.placeholderText = "Email";
            emailField.text = "";
            emailField.anchors.top = usernameField.bottom;
            emailField.anchors.topMargin = 1.5 * hifi.dimensions.contentSpacing.y;
            passwordField.text = "";
        }
        loginErrorMessage.visible = false;
        loginContainer.visible = true;
    }

    Item {
        id: mainContainer
        width: root.pane.width
        height: root.pane.height
        onHeightChanged: d.resize(); onWidthChanged: d.resize();

        Rectangle {
            id: opaqueRect
            height: parent.height
            width: parent.width
            opacity: 0.9
            color: "black"
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
            id: loggingInContainer
            width: parent.width
            height: parent.height
            onHeightChanged: d.resize(); onWidthChanged: d.resize();
            visible: false

            Item {
                id: loggingInHeader
                width: parent.width
                height: 0.5 * parent.height
                anchors {
                    top: parent.top
                }
                TextMetrics {
                    id: loggingInGlyphTextMetrics;
                    font: loggingInGlyph.font;
                    text: loggingInGlyph.text;
                }
                HifiStylesUit.HiFiGlyphs {
                    id: loggingInGlyph;
                    text: hifi.glyphs.steamSquare;
                    // Color
                    color: "white";
                    // Size
                    size: 31;
                    // Anchors
                    anchors.right: loggingInText.left;
                    anchors.rightMargin: signInBody.loggingInGlyphRightMargin
                    anchors.bottom: parent.bottom;
                    anchors.bottomMargin: hifi.dimensions.contentSpacing.y
                    // Alignment
                    horizontalAlignment: Text.AlignHCenter;
                    verticalAlignment: Text.AlignVCenter;
                    visible: loginDialog.isSteamRunning();
                }

                TextMetrics {
                    id: loggingInTextMetrics;
                    font: loggingInText.font;
                    text: loggingInText.text;
                }
                Text {
                    id: loggingInText;
                    width: loggingInTextMetrics.width
                    anchors.bottom: parent.bottom;
                    anchors.bottomMargin: hifi.dimensions.contentSpacing.y
                    anchors.left: parent.left;
                    anchors.leftMargin: (parent.width - loggingInTextMetrics.width) / 2
                    color: "white";
                    font.family: signInBody.fontFamily
                    font.pixelSize: signInBody.fontSize
                    font.bold: signInBody.fontBold
                    verticalAlignment: Text.AlignVCenter
                    horizontalAlignment: Text.AlignHCenter
                    text: "Logging in"
                }
            }
            Item {
                id: loggingInFooter
                width: parent.width
                height: 0.5 * parent.height
                anchors {
                    top: loggingInHeader.bottom
                }
                AnimatedImage {
                    id: linkAccountSpinner
                    source: "../../icons/loader-snake-64-w.gif"
                    width: 128
                    height: width
                    anchors.left: parent.left;
                    anchors.leftMargin: (parent.width - width) / 2;
                    anchors.top: parent.top
                    anchors.topMargin: hifi.dimensions.contentSpacing.y
                }
                TextMetrics {
                    id: loggedInGlyphTextMetrics;
                    font: loggedInGlyph.font;
                    text: loggedInGlyph.text;
                }
                HifiStylesUit.HiFiGlyphs {
                    id: loggedInGlyph;
                    text: hifi.glyphs.steamSquare;
                    // color
                    color: "white"
                    // Size
                    size: 78;
                    // Anchors
                    anchors.left: parent.left;
                    anchors.leftMargin: (parent.width - loggedInGlyph.size) / 2;
                    anchors.top: parent.top
                    anchors.topMargin: hifi.dimensions.contentSpacing.y
                    // Alignment
                    horizontalAlignment: Text.AlignHCenter;
                    verticalAlignment: Text.AlignVCenter;
                    visible: loginDialog.isSteamRunning();

                }
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
            visible: true

            Text {
                id: loginErrorMessage;
                anchors.bottom: emailField.top;
                anchors.bottomMargin: 2
                anchors.left: emailField.left;
                color: "red";
                font.family: signInBody.fontFamily
                font.pixelSize: 12
                verticalAlignment: Text.AlignVCenter
                horizontalAlignment: Text.AlignHCenter
                text: ""
                visible: false
            }

            HifiControlsUit.TextField {
                id: usernameField
                width: banner.width
                height: signInBody.textFieldHeight
                font.family: signInBody.fontFamily
                placeholderText: "Username"
                anchors {
                    top: parent.top
                    topMargin: 0.2 * parent.height
                    left: parent.left
                    leftMargin: (parent.width - usernameField.width) / 2
                }
                focus: !loginDialog.isLogIn
                visible: false
            }

            HifiControlsUit.TextField {
                id: emailField
                width: banner.width
                height: signInBody.textFieldHeight
                font.family: signInBody.fontFamily
                text: Settings.getValue("wallet/savedUsername", "");
                anchors {
                    top: parent.top
                    left: parent.left
                    leftMargin: (parent.width - emailField.width) / 2
                }
                focus: loginDialog.isLogIn
                placeholderText: "Username or Email"
                activeFocusOnPress: true
                onHeightChanged: d.resize(); onWidthChanged: d.resize();
            }
            HifiControlsUit.TextField {
                id: passwordField
                width: banner.width
                height: signInBody.textFieldHeight
                font.family: signInBody.fontFamily
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

                Item {
                    id: showPasswordContainer
                    z: 10
                    // width + image's rightMargin
                    width: showPasswordImage.width + 8
                    height: parent.height
                    anchors {
                        right: parent.right
                    }

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
                    signInBody.login()
                }
            }
            HifiControlsUit.CheckBox {
                id: autoLogoutCheckbox
                checked: !Settings.getValue("wallet/autoLogout", false);
                text: qsTr("Keep Me Logged In")
                boxSize: 18;
                labelFontFamily: signInBody.fontFamily
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
                    font.family: signInBody.fontFamily
                    font.pixelSize: signInBody.fontSize
                    font.capitalization: Font.AllUppercase;
                    font.bold: signInBody.fontBold
                    lineHeightMode: Text.ProportionalHeight
                }
                MouseArea {
                    id: cancelArea
                    anchors.fill: parent
                    acceptedButtons: Qt.LeftButton
                    onClicked: {
                        loginDialog.atSignIn = false;
                        bodyLoader.setSource("LinkAccountBody.qml", { "loginDialog": loginDialog, "root": root, "bodyLoader": bodyLoader });
                    }
                }
            }
            HifiControlsUit.Button {
                id: loginButtonAtSignIn
                width: d.minWidthButton
                height: d.minHeightButton
                text: qsTr("Log In")
                fontFamily: signInBody.fontFamily
                fontSize: signInBody.fontSize
                fontBold: signInBody.fontBold
                anchors {
                    top: cancelContainer.top
                    right: passwordField.right
                }

                onClicked: {
                    signInBody.login()
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
                    font.family: signInBody.fontFamily
                    font.pixelSize: 14

                    text: "<a href='https://highfidelity.com/users/password/new'> Can't access your account?</a>"

                    verticalAlignment: Text.AlignVCenter
                    horizontalAlignment: Text.AlignHCenter
                    linkColor: hifi.colors.blueAccent
                    onLinkActivated: loginDialog.openUrl(link)
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
        init(loginDialog.isLogIn)
    }

    Connections {
        target: loginDialog
        onHandleCreateFailed: {
            console.log("Create Failed")
            var error = "There was an unknown error while creating your account. Please try again later.";
            loadingSuccess(false, error);
        }
        onHandleCreateCompleted: {
            console.log("Create Completed")
        }
        onHandleSignupFailed: {
            console.log("Sign Up Failed")
            loadingSuccess(false, "");
        }
        onHandleSignupCompleted: {
            console.log("Sign Up Completed");
            loadingSuccess(true, "");
        }
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
                loginDialog.linkSteam();
            }
            loadingSuccess(true, "")
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
            if (loginDialog.isSteamRunning()) {
                bodyLoader.setSource("CompleteProfileBody.qml", { "loginDialog": loginDialog, "root": root, "bodyLoader": bodyLoader });
            }

            var error = "Username or password incorrect."
            loadingSuccess(false, error)
        }
        onHandleLinkCompleted: {
            console.log("Link Succeeded")
            loadingSuccess(true, "")
        }
        onHandleLinkFailed: {
            console.log("Link Failed")
            var error = "There was an unknown error while creating your account. Please try again later.";
            loadingSuccess(false, error);
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
            signInBody.login()
            break
        }
    }
}
