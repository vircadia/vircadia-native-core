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
            loginErrorMessage.text = errorString !== "" ? errorString : linkAccountBody.isLogIn ? "bad user/pass combo." : "unknown error.";
            loginErrorMessage.anchors.bottom = linkAccountBody.isLogIn ? emailField.top : usernameField.top;
            failureTimer.start();
            return;
        }
        if (loginDialog.isSteamRunning()) {
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
        loginDialog.atSignIn = signIn;
        loginDialog.isLogIn = isLogIn;

        bodyLoader.setSource("SignInBody.qml", { "loginDialog": loginDialog, "root": root, "bodyLoader": bodyLoader });

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
                    var poppedUp = Settings.getValue("loginDialogPoppedUp", false);
                    if (poppedUp) {

                        console.log("[ENCOURAGELOGINDIALOG]: user dismissed login screen")
                        var data = {
                            "action": "user dismissed login screen"
                        };
                        Settings.setValue("loginDialogPoppedUp", false);
                    }
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
        if (!visible) {
            return;
        }
    }
}
