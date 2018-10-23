//
//  LinkAccountBody.qml
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

import controlsUit 1.0 as HifiControlsUit
import stylesUit 1.0 as HifiStylesUit

import TabletScriptingInterface 1.0

Item {
    id: linkAccountBody
    clip: true
    height: root.pane.height
    width: root.pane.width
    property bool failAfterSignUp: false
    property string fontFamily: "Cairo"
    property int fontSize: 24
    property bool fontBold: true

    property bool keyboardEnabled: false
    property bool keyboardRaised: false
    property bool punctuationMode: false

    property bool isLogIn: false
    property bool atSignIn: false
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
        linkAccountBody.toggleLoading();
        if (linkAccountBody.isLogIn) {
            loginDialog.login(emailField.text, passwordField.text);
        } else {
            loginDialog.signup(usernameField.text, emailField.text, passwordField.text);
        }
    }
    function resetContainers() {
        splashContainer.visible = false;
        loggingInContainer.visible = false;
        loginContainer.visible = false;
    }

    function toggleLoading() {
        // For the process of logging in.
        linkAccountBody.resetContainers();
        loggingInContainer.visible = true;
        if (linkAccountBody.withSteam) {
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
            loginErrorMessage.text = errorString !== "" ? errorString : linkAccountBody.isLogIn ? "bad user/pass combo." : "unknown error.";
            loginErrorMessage.anchors.bottom = linkAccountBody.isLogIn ? emailField.top : usernameField.top;
            failureTimer.start();
            return;
        }
        if (linkAccountBody.withSteam) {
            // reset the flag.
            linkAccountBody.withSteam = false;
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

    function toggleSignIn(signIn, isLogIn) {
        // going to/from sign in/up dialog.
        linkAccountBody.atSignIn = signIn;
        linkAccountBody.isLogIn = isLogIn;
        if (signIn) {
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
        }

        splashContainer.visible = !signIn;
        topContainer.height = signIn ? root.pane.height : 0.6 * topContainer.height;
        bottomContainer.visible = !signIn;
        dismissTextContainer.visible = !signIn;
        topOpaqueRect.visible = signIn;
        loginContainer.visible = signIn;

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
                    anchors.rightMargin: linkAccountBody.loggingInGlyphRightMargin
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
                    font.family: linkAccountBody.fontFamily
                    font.pixelSize: linkAccountBody.fontSize
                    font.bold: linkAccountBody.fontBold
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
            visible: false

            Text {
                id: loginErrorMessage;
                anchors.bottom: emailField.top;
                anchors.bottomMargin: 2
                anchors.left: emailField.left;
                color: "red";
                font.family: linkAccountBody.fontFamily
                font.pixelSize: 12
                verticalAlignment: Text.AlignVCenter
                horizontalAlignment: Text.AlignHCenter
                text: ""
                visible: false
            }

            HifiControlsUit.TextField {
                id: usernameField
                width: banner.width
                font.family: linkAccountBody.fontFamily
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
                font.family: linkAccountBody.fontFamily
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
            }
            HifiControlsUit.TextField {
                id: passwordField
                width: banner.width
                font.family: linkAccountBody.fontFamily
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
                labelFontFamily: linkAccountBody.fontFamily
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
                    font.family: linkAccountBody.fontFamily
                    font.pixelSize: linkAccountBody.fontSize
                    font.capitalization: Font.AllUppercase;
                    font.bold: linkAccountBody.fontBold
                    lineHeightMode: Text.ProportionalHeight
                }
                MouseArea {
                    id: cancelArea
                    anchors.fill: parent
                    acceptedButtons: Qt.LeftButton
                    onClicked: {
                        toggleSignIn(false, true);
                        linkAccountBody.isLogIn = false;
                    }
                }
            }
            HifiControlsUit.Button {
                id: loginButtonAtSignIn
                width: d.minWidthButton
                height: d.minHeightButton
                text: qsTr("Log In")
                fontFamily: linkAccountBody.fontFamily
                fontSize: linkAccountBody.fontSize
                fontBold: linkAccountBody.fontBold
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
                    font.family: linkAccountBody.fontFamily
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
                lineHeight: 0.5
                color: "white"
                font.family: linkAccountBody.fontFamily
                font.pixelSize: 2 * linkAccountBody.fontSize
                font.bold: linkAccountBody.fontBold
                lineHeightMode: Text.ProportionalHeight
                horizontalAlignment: Text.AlignHCenter
            }

            HifiControlsUit.Button {
                id: signUpButton
                text: qsTr("Sign Up")
                width: d.minWidthButton
                height: d.minHeightButton
                color: hifi.buttons.blue
                fontFamily: linkAccountBody.fontFamily
                fontSize: linkAccountBody.fontSize
                fontBold: linkAccountBody.fontBold
                anchors {
                    bottom: parent.bottom
                    bottomMargin: 0.1 * parent.height
                    left: parent.left
                    leftMargin: (parent.width - d.minWidthButton) / 2
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
                width: d.minWidthButton
                height: d.minHeightButton
                text: qsTr("Log In")
                fontFamily: linkAccountBody.fontFamily
                fontSize: linkAccountBody.fontSize
                fontBold: linkAccountBody.fontBold
                anchors {
                    top: parent.top
                    topMargin: (parent.height  / 2) - loginButton.height
                    left: parent.left
                    leftMargin: (parent.width - loginButton.width) / 2
                }

                onClicked: {
                    toggleSignIn(true, true);
                }
            }
            HifiControlsUit.Button {
                id: steamLoginButton;
                width: d.minWidthButton
                height: d.minHeightButton
                color: hifi.buttons.black;
                anchors {
                    top: loginButton.bottom
                    topMargin: hifi.dimensions.contentSpacing.y
                    left: parent.left
                    leftMargin: (parent.width - steamLoginButton.width) / 2
                }
                text: qsTr("Steam Log In")
                fontFamily: linkAccountBody.fontFamily
                fontSize: linkAccountBody.fontSize
                fontBold: linkAccountBody.fontBold
                buttonGlyph: hifi.glyphs.steamSquare
                buttonGlyphRightMargin: 10
                onClicked: {
                    linkAccountBody.withSteam = true;
                    linkAccountBody.toggleLoading();
                    loginDialog.linkSteam();
                }
                visible: loginDialog.isSteamRunning();
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
                font.family: linkAccountBody.fontFamily
                font.pixelSize: linkAccountBody.fontSize
                font.bold: linkAccountBody.fontBold
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
        onHandleCreateFailed: {
            console.log("Create Failed")
        }
        onHandleCreateCompleted: {
            console.log("Create Completed")
        }
        onHandleSignupFailed: {
            console.log("Sign Up Failed")
            loadingSuccess(false, errorString)
        }
        onHandleSignupCompleted: {
            console.log("Sign Up Completed")
            loadingSuccess(true)
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
            loadingSuccess(true)
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
            loadingSuccess(false)
        }
        onHandleLinkCompleted: {
            console.log("Link Succeeded")
            loadingSuccess(true)
        }
        onHandleLinkFailed: {
            console.log("Link Failed")
            var error = "There was an unknown error while creating your account. Please try again later.";
            loadingSuccess(false, error);
        }
    }

    Keys.onPressed: {
        if (!visible && !atSignIn) {
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
