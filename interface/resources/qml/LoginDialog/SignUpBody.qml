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
import QtQuick 2.7
import QtQuick.Controls 1.4
import QtQuick.Controls.Styles 1.4 as OriginalStyles

import controlsUit 1.0 as HifiControlsUit
import stylesUit 1.0 as HifiStylesUit
import TabletScriptingInterface 1.0

Item {
    z: -2
    id: signUpBody
    clip: true
    height: root.height
    width: root.width
    readonly property string termsContainerText: qsTr("By signing up, you agree to Vircadia's Terms of Service")
    property int textFieldHeight: 31
    property string fontFamily: "Raleway"
    property int fontSize: 15
    property int textFieldFontSize: 18
    property bool fontBold: true
    readonly property var passwordImageRatio: 16 / 23

    property bool keyboardEnabled: false
    property bool keyboardRaised: false
    property bool punctuationMode: false

    onKeyboardRaisedChanged: d.resize();

    property string errorString: errorString
    property bool lostFocus: false

    readonly property bool loginDialogPoppedUp: loginDialog.getLoginDialogPoppedUp()

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
            var targetHeight =  hifi.dimensions.contentSpacing.y + mainContainer.height +
                    4 * hifi.dimensions.contentSpacing.y;

            var newWidth = Math.max(d.minWidth, Math.min(d.maxWidth, targetWidth));
            if (!isNaN(newWidth)) {
                parent.width = root.width = newWidth;
            }

            parent.height = root.height = Math.max(d.minHeight, Math.min(d.maxHeight, targetHeight)) + hifi.dimensions.contentSpacing.y;
        }
    }

    function signup() {
        loginDialog.signup(emailField.text, usernameField.text, passwordField.text);
    }

    function init() {
        // going to/from sign in/up dialog.
        emailField.placeholderText = "Email";
        emailField.text = "";
        emailField.anchors.top = usernameField.bottom;
        emailField.anchors.topMargin = 1.5 * hifi.dimensions.contentSpacing.y;
        passwordField.text = "";
        usernameField.forceActiveFocus();
        root.text = "";
        root.isPassword = false;
        loginContainer.visible = true;
    }

    Item {
        id: mainContainer
        width: root.width
        height: root.height
        onHeightChanged: d.resize(); onWidthChanged: d.resize();

        Item {
            id: loginContainer
            width: usernameField.width
            height: parent.height
            anchors {
                top: parent.top
                topMargin: root.bannerHeight + 0.25 * parent.height
                left: parent.left
                leftMargin: (parent.width - usernameField.width) / 2
            }
            visible: true

            Item {
                id: errorContainer
                width: parent.width
                height: loginErrorMessageTextMetrics.height
                anchors {
                    bottom: usernameField.top;
                    bottomMargin: hifi.dimensions.contentSpacing.y;
                    left: usernameField.left;
                }
                TextMetrics {
                    id: loginErrorMessageTextMetrics
                    font: loginErrorMessage.font
                    text: loginErrorMessage.text
                }
                Text {
                    id: loginErrorMessage;
                    width: root.bannerWidth
                    color: "red";
                    font.family: signUpBody.fontFamily
                    font.pixelSize: 18
                    font.bold: signUpBody.fontBold
                    verticalAlignment: Text.AlignVCenter
                    horizontalAlignment: Text.AlignHCenter
                    text: ""
                    visible: false
                }
            }

            HifiControlsUit.TextField {
                id: usernameField
                width: root.bannerWidth
                height: signUpBody.textFieldHeight
                placeholderText: "Username"
                font.pixelSize: signUpBody.textFieldFontSize
                styleRenderType: Text.QtRendering
                anchors {
                    top: parent.top
                    topMargin: loginErrorMessage.height
                }
                Keys.onPressed: {
                    if (!usernameField.visible) {
                        return;
                    }
                    switch (event.key) {
                        case Qt.Key_Tab:
                            event.accepted = true;
                            if (event.modifiers === Qt.ShiftModifier) {
                                passwordField.focus = true;
                            } else {
                                emailField.focus = true;
                            }
                            break;
                        case Qt.Key_Backtab:
                            event.accepted = true;
                            passwordField.focus = true;
                            break;
                        case Qt.Key_Enter:
                        case Qt.Key_Return:
                            event.accepted = true;
                            if (keepMeLoggedInCheckbox.checked) {
                                Settings.setValue("keepMeLoggedIn/savedUsername", usernameField.text);
                            }
                            signUpBody.signup();
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
                id: emailField
                width: root.bannerWidth
                height: signUpBody.textFieldHeight
                anchors {
                    top: parent.top
                }
                placeholderText: "Username or Email"
                font.pixelSize: signUpBody.textFieldFontSize
                styleRenderType: Text.QtRendering
                activeFocusOnPress: true
                Keys.onPressed: {
                    switch (event.key) {
                        case Qt.Key_Tab:
                            event.accepted = true;
                            if (event.modifiers === Qt.ShiftModifier) {
                                if (usernameField.visible) {
                                    usernameField.focus = true;
                                    break;
                                }
                            }
                            passwordField.focus = true;
                            break;
                        case Qt.Key_Backtab:
                            event.accepted = true;
                            usernameField.focus = true;
                            break;
                        case Qt.Key_Enter:
                        case Qt.Key_Return:
                            event.accepted = true;
                            if (keepMeLoggedInCheckbox.checked) {
                                Settings.setValue("keepMeLoggedIn/savedUsername", usernameField.text);
                            }
                            signUpBody.signup();
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
                height: signUpBody.textFieldHeight
                placeholderText: "Password (min. 6 characters)"
                font.pixelSize: signUpBody.textFieldFontSize
                styleRenderType: Text.QtRendering
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
                            event.accepted = true;
                            if (event.modifiers === Qt.ShiftModifier) {
                                emailField.focus = true;
                            } else if (usernameField.visible) {
                                usernameField.focus = true;
                            } else {
                                emailField.focus = true;
                            }
                            break;
                        case Qt.Key_Backtab:
                            event.accepted = true;
                            emailField.focus = true;
                            break;
                    case Qt.Key_Enter:
                    case Qt.Key_Return:
                        event.accepted = true;
                        if (keepMeLoggedInCheckbox.checked) {
                            Settings.setValue("keepMeLoggedIn/savedUsername", usernameField.text);
                        }
                        signUpBody.signup();
                        break;
                    }
                }
            }
            HifiControlsUit.CheckBox {
                id: keepMeLoggedInCheckbox
                checked: Settings.getValue("keepMeLoggedIn", false);
                text: qsTr("Keep Me Logged In");
                boxSize: 18;
                labelFontFamily: signUpBody.fontFamily
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

            TextMetrics {
                id: cancelButtonTextMetrics
                font: loginErrorMessage.font
                text: cancelButton.text
            }
            HifiControlsUit.Button {
                id: cancelButton
                width: (emailField.width - hifi.dimensions.contentSpacing.x) / 2
                height: d.minHeightButton
                anchors {
                    top: keepMeLoggedInCheckbox.bottom
                    topMargin: hifi.dimensions.contentSpacing.y
                    left: parent.left
                }
                color: hifi.buttons.noneBorderlessWhite
                text: qsTr("CANCEL")
                fontFamily: signUpBody.fontFamily
                fontSize: signUpBody.fontSize
                fontBold: signUpBody.fontBold
                onClicked: {
                    if (signUpBody.loginDialogPoppedUp) {
                        var data = {
                            "action": "user clicked cancel button at sign up screen"
                        }
                        UserActivityLogger.logAction("encourageLoginDialog", data);
                    }
                    bodyLoader.setSource("LinkAccountBody.qml", { "loginDialog": loginDialog, "root": root, "bodyLoader": bodyLoader, "linkSteam": false });
                }
            }
            HifiControlsUit.Button {
                id: signUpButton
                width: (emailField.width - hifi.dimensions.contentSpacing.x) / 2
                height: d.minHeightButton
                color: hifi.buttons.blue
                text: qsTr("Sign Up")
                fontFamily: signUpBody.fontFamily
                fontSize: signUpBody.fontSize
                fontBold: signUpBody.fontBold
                anchors {
                    top: cancelButton.top
                    right: passwordField.right
                }

                onClicked: {
                    if (signUpBody.loginDialogPoppedUp) {
                        var data = {
                            "action": "user clicked sign up button"
                        }
                        UserActivityLogger.logAction("encourageLoginDialog", data);
                    }
                    signUpBody.signup();
                }
            }
            Item {
                id: termsContainer
                width: parent.width
                height: termsTextMetrics.height
                anchors {
                    top: signUpButton.bottom
                    horizontalCenter: parent.horizontalCenter
                    topMargin: 2 * hifi.dimensions.contentSpacing.y
                    left: parent.left
                }
                TextMetrics {
                    id: termsTextMetrics
                    font: termsText.font
                    text: signUpBody.termsContainerText
                    Component.onCompleted: {
                        // with the link.
                        termsText.text = qsTr("By signing up, you agree to <a href='https://vircadia.com/termsofservice'>Vircadia's Terms of Service</a>")
                    }
                }

                HifiStylesUit.InfoItem {
                    id: termsText
                    text: signUpBody.termsContainerText
                    font.family: signUpBody.fontFamily
                    font.pixelSize: signUpBody.fontSize
                    font.bold: signUpBody.fontBold
                    wrapMode: Text.WordWrap
                    color: hifi.colors.white
                    linkColor: hifi.colors.blueAccent
                    lineHeight: 1
                    lineHeightMode: Text.ProportionalHeight

                    onLinkActivated: Window.openUrl(link);

                    Component.onCompleted: {
                        if (termsTextMetrics.width > root.bannerWidth) {
                            termsText.width = root.bannerWidth;
                            termsText.wrapMode = Text.WordWrap;
                            additionalText.verticalAlignment = Text.AlignLeft;
                            additionalText.horizontalAlignment = Text.AlignLeft;
                            termsContainer.height = (termsTextMetrics.width / root.bannerWidth) * termsTextMetrics.height;
                            termsContainer.anchors.left = buttons.left;
                        } else {
                            termsText.anchors.centerIn = termsContainer;
                        }
                    }
                }
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
    }

    Keys.onPressed: {
        if (!visible) {
            return;
        }

        switch (event.key) {
            case Qt.Key_Enter:
            case Qt.Key_Return:
                event.accepted = true;
                Settings.setValue("keepMeLoggedIn/savedUsername", usernameField.text);
                signUpBody.login();
                break;
        }
    }
    Connections {
        target: loginDialog
        onHandleSignupCompleted: {
            console.log("Sign Up Completed");

            if (signUpBody.loginDialogPoppedUp) {
                var data = {
                    "action": "user signed up successfully"
                }
                UserActivityLogger.logAction("encourageLoginDialog", data);
            }

            loginDialog.login(usernameField.text, passwordField.text);
            bodyLoader.setSource("LoggingInBody.qml", { "loginDialog": loginDialog, "root": root, "bodyLoader": bodyLoader, "withSteam": false, "linkSteam": false });
        }
        onHandleSignupFailed: {
            console.log("Sign Up Failed")

            if (signUpBody.loginDialogPoppedUp) {
                var data = {
                    "action": "user signed up unsuccessfully"
                }
                UserActivityLogger.logAction("encourageLoginDialog", data);
            }

            if (errorString !== "") {
                loginErrorMessage.visible = true;
                var errorLength = errorString.split(/\r\n|\r|\n/).length;
                var errorStringEdited = errorString.replace(/[\n\r]+/g, "\n");
                loginErrorMessage.text = errorStringEdited;
                if (errorLength > 1.0) {
                    loginErrorMessage.width = root.bannerWidth;
                    loginErrorMessage.wrapMode = Text.WordWrap;
                    loginErrorMessage.verticalAlignment = Text.AlignLeft;
                    loginErrorMessage.horizontalAlignment = Text.AlignLeft;
                    errorContainer.height = errorLength * loginErrorMessageTextMetrics.height;
                }
                errorContainer.anchors.bottom = usernameField.top;
                errorContainer.anchors.bottomMargin = hifi.dimensions.contentSpacing.y;
                errorContainer.anchors.left = usernameField.left;
            }
        }
        onFocusEnabled: {
            if (!signUpBody.lostFocus) {
                Qt.callLater(function() {
                    emailField.forceActiveFocus();
                });
            }
        }
        onFocusDisabled: {
            signUpBody.lostFocus = !root.isTablet && !root.isOverlay;
            if (signUpBody.lostFocus) {
                Qt.callLater(function() {
                    usernameField.focus = false;
                    emailField.focus = false;
                    passwordField.focus = false;
                });
            }
        }
    }
}
