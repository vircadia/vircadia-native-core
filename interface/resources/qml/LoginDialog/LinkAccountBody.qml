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

import "." as LoginDialog

import controlsUit 1.0 as HifiControlsUit
import stylesUit 1.0 as HifiStylesUit
import TabletScriptingInterface 1.0

Item {
    z: -2
    id: linkAccountBody
    clip: true
    height: root.height
    width: root.width
    property int textFieldHeight: 31
    property string fontFamily: "Raleway"
    property int fontSize: 15
    property int textFieldFontSize: 18
    property bool fontBold: true
    readonly property var passwordImageRatio: 16 / 23

    property bool keyboardEnabled: false
    property bool keyboardRaised: false
    property bool punctuationMode: false

    property bool withSteam: false
    property bool linkSteam: linkSteam
    property bool withOculus: false
    property string errorString: errorString
    property bool lostFocus: false

    QtObject {
        id: d
        readonly property int minWidth: 480
        readonly property int minWidthButton: !root.isTablet ? 256 : 174
        property int maxWidth: root.width
        readonly property int minHeight: 120
        readonly property int minHeightButton: 36
        property int maxHeight: root.height

        function resize() {
            maxWidth = root.width;
            maxHeight = root.height;
            var targetWidth = Math.max(titleWidth, mainContainer.width);
            var targetHeight =  hifi.dimensions.contentSpacing.y + mainContainer.height + 4 * hifi.dimensions.contentSpacing.y;

            var newWidth = Math.max(d.minWidth, Math.min(d.maxWidth, targetWidth));
            if (!isNaN(newWidth)) {
                parent.width = root.width = newWidth;
            }

            parent.height = root.height = Math.max(d.minHeight, Math.min(d.maxHeight, targetHeight)) + hifi.dimensions.contentSpacing.y;
        }
    }

    function login() {
        loginDialog.login(emailField.text, passwordField.text);
        bodyLoader.setSource("LoggingInBody.qml", { "loginDialog": loginDialog, "root": root, "bodyLoader": bodyLoader, "withSteam": linkAccountBody.withSteam, "withOculus": linkAccountBody.withOculus, "linkSteam": linkAccountBody.linkSteam });
    }

    function init() {
        // going to/from sign in/up dialog.
        loginDialog.isLogIn = true;
        loginErrorMessage.text = linkAccountBody.errorString;
        loginErrorMessage.visible = (linkAccountBody.errorString  !== "");
        loginButton.text = !linkAccountBody.linkSteam ? "Log In" : "Link Account";
        loginButton.color = hifi.buttons.blue;
        emailField.placeholderText = "Username or Email";
        var savedUsername = Settings.getValue("keepMeLoggedIn/savedUsername", "");
        emailField.text = keepMeLoggedInCheckbox.checked ? savedUsername === "Unknown user" ? "" : savedUsername : "";
        if (linkAccountBody.linkSteam) {
            steamInfoText.anchors.top = passwordField.bottom;
            keepMeLoggedInCheckbox.anchors.top = steamInfoText.bottom;
            loginButton.width = (passwordField.width - hifi.dimensions.contentSpacing.x) / 2;
            loginButton.anchors.right = emailField.right;
        } else {
            loginButton.anchors.left = emailField.left;
        }
        loginContainer.visible = true;
    }

    Item {
        z: 10
        id: mainContainer
        width: root.width
        height: root.height
        onHeightChanged: d.resize(); onWidthChanged: d.resize();

        LoginDialog.LoginDialogLightbox {
            id: lightboxPopup;
            visible: false;
            anchors.fill: parent;
        }

        Item {
            id: loginContainer
            width: emailField.width
            height: errorContainer.height + emailField.height + passwordField.height + 5.5 * hifi.dimensions.contentSpacing.y +
                keepMeLoggedInCheckbox.height + loginButton.height + cantAccessTextMetrics.height + continueButton.height + steamInfoTextMetrics.height
            anchors {
                top: parent.top
                topMargin: root.bannerHeight + 0.25 * parent.height
                left: parent.left
                leftMargin: (parent.width - emailField.width) / 2
            }

            Item {
                id: errorContainer
                width: loginErrorMessageTextMetrics.width
                height: loginErrorMessageTextMetrics.height
                anchors {
                    bottom: emailField.top;
                    bottomMargin: hifi.dimensions.contentSpacing.y;
                    left: emailField.left;
                }
                TextMetrics {
                    id: loginErrorMessageTextMetrics
                    font: loginErrorMessage.font
                    text: loginErrorMessage.text
                }
                Text {
                    id: loginErrorMessage;
                    color: "red";
                    font.family: linkAccountBody.fontFamily
                    font.pixelSize: linkAccountBody.textFieldFontSize
                    font.bold: linkAccountBody.fontBold
                    verticalAlignment: Text.AlignVCenter
                    horizontalAlignment: Text.AlignHCenter
                    text: ""
                    visible: false
                }
            }

            HifiControlsUit.TextField {
                id: emailField
                width: root.bannerWidth
                height: linkAccountBody.textFieldHeight
                font.pixelSize: linkAccountBody.textFieldFontSize
                styleRenderType: Text.QtRendering
                anchors {
                    top: parent.top
                    topMargin: errorContainer.height
                }
                placeholderText: "Username or Email"
                activeFocusOnPress: true
                Keys.onPressed: {
                    switch (event.key) {
                        case Qt.Key_Tab:
                            event.accepted = true;
                            passwordField.focus = true;
                            break;
                        case Qt.Key_Backtab:
                            event.accepted = true;
                            passwordField.focus = true;
                            break;
                        case Qt.Key_Enter:
                        case Qt.Key_Return:
                            event.accepted = true;
                            if (keepMeLoggedInCheckbox.checked) {
                                Settings.setValue("keepMeLoggedIn/savedUsername", emailField.text);
                            }
                            linkAccountBody.login();
                            break;
                    }
                }
                onFocusChanged: {
                    root.text = "";
                    if (focus) {
                        root.isPassword = false;
                    }
                }
            }
            HifiControlsUit.TextField {
                id: passwordField
                width: root.bannerWidth
                height: linkAccountBody.textFieldHeight
                font.pixelSize: linkAccountBody.textFieldFontSize
                styleRenderType: Text.QtRendering
                placeholderText: "Password"
                activeFocusOnPress: true
                echoMode: passwordFieldMouseArea.showPassword ? TextInput.Normal : TextInput.Password
                anchors {
                    top: emailField.bottom
                    topMargin: 1.5 * hifi.dimensions.contentSpacing.y
                }

                onFocusChanged: {
                    root.text = "";
                    root.isPassword = focus;
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
                        width: passwordField.height * passwordImageRatio
                        height: passwordField.height * passwordImageRatio
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
                Keys.onPressed: {
                    switch (event.key) {
                        case Qt.Key_Tab:
                        case Qt.Key_Backtab:
                            event.accepted = true;
                            emailField.focus = true;
                            break;
                        case Qt.Key_Enter:
                        case Qt.Key_Return:
                            event.accepted = true;
                            if (keepMeLoggedInCheckbox.checked) {
                                Settings.setValue("keepMeLoggedIn/savedUsername", emailField.text);
                            }
                            linkAccountBody.login();
                            break;
                    }
                }
            }
            HifiControlsUit.CheckBox {
                id: keepMeLoggedInCheckbox
                checked: Settings.getValue("keepMeLoggedIn", false);
                text: qsTr("Keep Me Logged In");
                boxSize: 18;
                labelFontFamily: linkAccountBody.fontFamily
                labelFontSize: 18;
                color: hifi.colors.white;
                anchors {
                    top: passwordField.bottom;
                    topMargin: hifi.dimensions.contentSpacing.y;
                    left: passwordField.left;
                }
                onCheckedChanged: {
                    Settings.setValue("keepMeLoggedIn", checked);
                    if (keepMeLoggedInCheckbox.checked) {
                        Settings.setValue("keepMeLoggedIn/savedUsername", emailField.text);
                    } else {
                        Settings.setValue("keepMeLoggedIn/savedUsername", "");
                    }
                }
                Component.onCompleted: {
                    keepMeLoggedInCheckbox.checked = !Account.loggedIn;
                }
            }
            HifiControlsUit.Button {
                id: cancelButton
                width: (passwordField.width - hifi.dimensions.contentSpacing.x) / 2;
                height: d.minHeightButton
                text: qsTr("Cancel")
                fontFamily: linkAccountBody.fontFamily
                fontSize: linkAccountBody.fontSize
                fontBold: linkAccountBody.fontBold
                color: hifi.buttons.noneBorderlessWhite;
                visible: linkAccountBody.linkSteam
                anchors {
                    top: keepMeLoggedInCheckbox.bottom
                    topMargin: hifi.dimensions.contentSpacing.y
                }
                onClicked: {
                    bodyLoader.setSource("CompleteProfileBody.qml", { "loginDialog": loginDialog, "root": root, "bodyLoader": bodyLoader, "withSteam": linkAccountBody.withSteam, "errorString": "" });
                }
            }
            HifiControlsUit.Button {
                id: loginButton
                width: passwordField.width
                height: d.minHeightButton
                text: qsTr("Log In")
                fontFamily: linkAccountBody.fontFamily
                fontSize: linkAccountBody.fontSize
                fontBold: linkAccountBody.fontBold
                anchors {
                    top: keepMeLoggedInCheckbox.bottom
                    topMargin: hifi.dimensions.contentSpacing.y
                }
                onClicked: {
                    linkAccountBody.login()
                }
            }
            TextMetrics {
                id: steamInfoTextMetrics
                font: steamInfoText.font
                text: steamInfoText.text
            }
            Text {
                id: steamInfoText
                width: root.bannerWidth
                visible: linkAccountBody.linkSteam
                anchors {
                    top: loginButton.bottom
                    topMargin: hifi.dimensions.contentSpacing.y
                    left: emailField.left
                }

                font.family: linkAccountBody.fontFamily
                font.pixelSize: linkAccountBody.textFieldFontSize
                color: "white"
                text: qsTr("Your Steam account information will not be exposed to others.");
                verticalAlignment: Text.AlignVCenter
                horizontalAlignment: Text.AlignHCenter
                Component.onCompleted: {
                    if (steamInfoTextMetrics.width > root.bannerWidth) {
                        steamInfoText.wrapMode = Text.WordWrap;
                    }
                }
            }
            TextMetrics {
                id: cantAccessTextMetrics
                font: cantAccessText.font
                text: "Can't access your account?"
            }
            HifiStylesUit.ShortcutText {
                id: cantAccessText
                z: 10
                visible: !linkAccountBody.linkSteam
                anchors {
                    top: loginButton.bottom
                    topMargin: hifi.dimensions.contentSpacing.y
                    left: emailField.left
                }
                font.family: linkAccountBody.fontFamily
                font.pixelSize: linkAccountBody.textFieldFontSize
                font.bold: linkAccountBody.fontBold

                text: "<a href='https://highfidelity.com/users/password/new'> Can't access your account?</a>"

                verticalAlignment: Text.AlignVCenter
                horizontalAlignment: Text.AlignHCenter
                linkColor: hifi.colors.blueAccent
                onLinkActivated: {
                    Tablet.playSound(TabletEnums.ButtonClick);
                    loginDialog.openUrl(link);
                    lightboxPopup.titleText = "Can't Access Account";
                    lightboxPopup.bodyText = lightboxPopup.cantAccessBodyText;
                    lightboxPopup.button2text = "CLOSE";
                    lightboxPopup.button2method = function() {
                        lightboxPopup.visible = false;
                    }
                    lightboxPopup.visible = true;
                    // bodyLoader.setSource("CantAccessBody.qml", { "loginDialog": loginDialog, "root": root, "bodyLoader": bodyLoader });
                }
            }
            HifiControlsUit.Button {
                id: continueButton;
                width: emailField.width;
                height: d.minHeightButton
                color: hifi.buttons.none;
                anchors {
                    top: cantAccessText.bottom
                    topMargin: hifi.dimensions.contentSpacing.y
                    left: emailField.left
                }
                text: qsTr("CONTINUE WITH STEAM")
                fontFamily: linkAccountBody.fontFamily
                fontSize: linkAccountBody.fontSize
                fontBold: linkAccountBody.fontBold
                buttonGlyph: hifi.glyphs.steamSquare
                buttonGlyphSize: 24
                buttonGlyphRightMargin: 10
                onClicked: {
                    // if (loginDialog.isOculusStoreRunning()) {
                    //     linkAccountBody.withOculus = true;
                    //     loginDialog.loginThroughSteam();
                    // } else
                    if (loginDialog.isSteamRunning()) {
                        linkAccountBody.withSteam = true;
                        loginDialog.loginThroughSteam();
                    }

                    bodyLoader.setSource("LoggingInBody.qml", { "loginDialog": loginDialog, "root": root, "bodyLoader": bodyLoader,
                        "withSteam": linkAccountBody.withSteam, "withOculus": linkAccountBody.withOculus, "linkSteam": linkAccountBody.linkSteam });
                }
                Component.onCompleted: {
                    if (linkAccountBody.linkSteam) {
                        continueButton.visible = false;
                        return;
                    }
                    // if (loginDialog.isOculusStoreRunning()) {
                    //     continueButton.text = qsTr("CONTINUE WITH OCULUS");
                    //     continueButton.buttonGlyph = hifi.glyphs.oculus;
                    // } else
                    if (loginDialog.isSteamRunning()) {
                        continueButton.text = qsTr("CONTINUE WITH STEAM");
                        continueButton.buttonGlyph = hifi.glyphs.steamSquare;
                    } else {
                        continueButton.visible = false;
                    }
                }
            }
        }
        Item {
            id: signUpContainer
            width: loginContainer.width
            height: signUpTextMetrics.height
            visible: !linkAccountBody.linkSteam
            anchors {
                left: loginContainer.left
                top: loginContainer.bottom
                topMargin: 0.15 * parent.height
            }
            TextMetrics {
                id: signUpTextMetrics
                font: signUpText.font
                text: signUpText.text
            }
            Text {
                id: signUpText
                text: qsTr("Don't have an account?")
                anchors {
                    left: parent.left
                }
                lineHeight: 1
                color: "white"
                font.family: linkAccountBody.fontFamily
                font.pixelSize: linkAccountBody.textFieldFontSize
                font.bold: linkAccountBody.fontBold
                verticalAlignment: Text.AlignVCenter
                horizontalAlignment: Text.AlignHCenter
            }

            HifiStylesUit.ShortcutText {
                id: signUpShortcutText
                z: 10
                font.family: linkAccountBody.fontFamily
                font.pixelSize: linkAccountBody.textFieldFontSize
                font.bold: linkAccountBody.fontBold
                anchors {
                     left: signUpText.right
                     leftMargin: hifi.dimensions.contentSpacing.x
                }

                text: "<a href='https://highfidelity.com'>Sign Up</a>"

                linkColor: hifi.colors.blueAccent
                onLinkActivated: {
                    Tablet.playSound(TabletEnums.ButtonClick);
                    bodyLoader.setSource("SignUpBody.qml", { "loginDialog": loginDialog, "root": root, "bodyLoader": bodyLoader,
                        "errorString": "", "linkSteam": linkAccountBody.linkSteam });
                }
            }
        }
        TextMetrics {
            id: dismissButtonTextMetrics
            font: loginErrorMessage.font
            text: dismissButton.text
        }
        HifiControlsUit.Button {
            id: dismissButton
            width: dismissButtonTextMetrics.width
            height: d.minHeightButton
            anchors {
                bottom: parent.bottom
                right: parent.right
                margins: 3 * hifi.dimensions.contentSpacing.y
            }
            color: hifi.buttons.noneBorderlessWhite
            text: qsTr("No thanks, take me in-world! >")
            fontCapitalization: Font.MixedCase
            fontFamily: linkAccountBody.fontFamily
            fontSize: linkAccountBody.fontSize
            fontBold: linkAccountBody.fontBold
            visible: loginDialog.getLoginDialogPoppedUp() && !linkAccountBody.linkSteam;
            onClicked: {
                if (loginDialog.getLoginDialogPoppedUp()) {
                    console.log("[ENCOURAGELOGINDIALOG]: user dismissed login screen")
                    var data = {
                        "action": "user dismissed login screen"
                    };
                    UserActivityLogger.logAction("encourageLoginDialog", data);
                    loginDialog.dismissLoginDialog();
                }
                root.tryDestroy();
            }
        }
    }

    Connections {
        target: loginDialog
        onFocusEnabled: {
            if (!linkAccountBody.lostFocus) {
                Qt.callLater(function() {
                    emailField.forceActiveFocus();
                });
            }
        }
        onFocusDisabled: {
            linkAccountBody.lostFocus = !root.isTablet && !root.isOverlay;
            if (linkAccountBody.lostFocus) {
                Qt.callLater(function() {
                    emailField.focus = false;
                    passwordField.focus = false;
                });
            }
        }
    }

    Component.onCompleted: {
        //but rise Tablet's one instead for Tablet interface
        root.keyboardEnabled = HMD.active;
        root.keyboardRaised = Qt.binding( function() { return keyboardRaised; })
        root.text = "";
        d.resize();
        init();
        Qt.callLater(function() {
            emailField.forceActiveFocus();
        });
    }

    Keys.onPressed: {
        if (!visible) {
            return;
        }

        switch (event.key) {
            case Qt.Key_Enter:
            case Qt.Key_Return:
                event.accepted = true;
                Settings.setValue("keepMeLoggedIn/savedUsername", emailField.text);
                linkAccountBody.login();
                break;
        }
    }
}
