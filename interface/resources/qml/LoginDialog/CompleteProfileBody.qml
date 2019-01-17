//
//  CompleteProfileBody.qml
//
//  Created by Clement on 7/18/16
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import Hifi 1.0
import QtQuick 2.4
import QtQuick.Controls.Styles 1.4 as OriginalStyles

import controlsUit 1.0 as HifiControlsUit
import stylesUit 1.0 as HifiStylesUit
import TabletScriptingInterface 1.0

Item {
    id: completeProfileBody
    clip: true
    width: root.width
    height: root.height
    readonly property string termsContainerText: qsTr("By creating this user profile, you agree to High Fidelity's Terms of Service")
    readonly property string termsContainerOculusText: qsTr("By signing up, you agree to High Fidelity's Terms of Service")
    readonly property int textFieldHeight: 31
    readonly property string fontFamily: "Raleway"
    readonly property int fontSize: 15
    readonly property bool fontBold: true
    readonly property int textFieldFontSize: 18
    readonly property var passwordImageRatio: 16 / 23

    property bool withOculus: withOculus
    property bool withSteam: withSteam
    property string errorString: errorString

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
            if (root.isTablet === false) {
                var targetWidth = Math.max(Math.max(titleWidth, Math.max(additionalTextContainer.width,
                                                                termsContainer.width)), mainContainer.width)
                parent.width = root.width = Math.max(d.minWidth, Math.min(d.maxWidth, targetWidth))
            }
            var targetHeight = Math.max(5 * hifi.dimensions.contentSpacing.y + d.minHeightButton + additionalTextContainer.height + termsContainer.height, d.maxHeight)
            parent.height = root.height = Math.max(d.minHeight, Math.min(d.maxHeight, targetHeight))
        }
    }

    Item {
        id: mainContainer
        width: root.width
        height: root.height
        onHeightChanged: d.resize(); onWidthChanged: d.resize();

        Item {
            id: contentItem
            width: parent.width
            height: errorContainer.height + fields.height + buttons.height + additionalTextContainer.height +
                termsContainer.height
            anchors.top: parent.top
            anchors.topMargin: root.bannerHeight + 0.25 * parent.height
            anchors.left: parent.left

            Item {
                id: errorContainer
                width: parent.width
                height: loginErrorMessageTextMetrics.height
                anchors {
                    bottom: completeProfileBody.withOculus ? fields.top : buttons.top;
                    bottomMargin: 1.5 * hifi.dimensions.contentSpacing.y;
                    left: buttons.left;
                }
                TextMetrics {
                    id: loginErrorMessageTextMetrics
                    font: loginErrorMessage.font
                    text: loginErrorMessage.text
                }
                Text {
                    id: loginErrorMessage;
                    color: "red";
                    width: root.bannerWidth;
                    font.family: completeProfileBody.fontFamily
                    font.pixelSize: 18
                    font.bold: completeProfileBody.fontBold
                    verticalAlignment: Text.AlignVCenter
                    horizontalAlignment: Text.AlignHCenter
                    text: completeProfileBody.errorString
                    visible: true
                }
                Component.onCompleted: {
                    if (loginErrorMessageTextMetrics.width > root.bannerWidth) {
                        loginErrorMessage.wrapMode = Text.WordWrap;
                        loginErrorMessage.verticalAlignment = Text.AlignLeft;
                        loginErrorMessage.horizontalAlignment = Text.AlignLeft;
                        errorContainer.height = (loginErrorMessageTextMetrics.width / root.bannerWidth) * loginErrorMessageTextMetrics.height;
                    } else {
                        loginErrorMessage.wrapMode = Text.NoWrap;
                        errorContainer.height = loginErrorMessageTextMetrics.height;
                    }
                }
            }

            Item {
                id: fields
                width: root.bannerWidth
                height: 3 * completeProfileBody.textFieldHeight + 2 * hifi.dimensions.contentSpacing.y
                visible: completeProfileBody.withOculus
                anchors {
                    left: parent.left
                    leftMargin: (parent.width - root.bannerWidth) / 2
                    bottom: buttons.top
                    bottomMargin: hifi.dimensions.contentSpacing.y
                }

                HifiControlsUit.TextField {
                    id: usernameField
                    width: root.bannerWidth
                    height: completeProfileBody.textFieldHeight
                    placeholderText: "Username"
                    font.pixelSize: completeProfileBody.textFieldFontSize
                    styleRenderType: Text.QtRendering
                    anchors {
                        top: parent.top
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
                                loginDialog.createAccountFromOculus(emailField.text, usernameField.text, passwordField.text);
                                bodyLoader.setSource("LoggingInBody.qml", { "loginDialog": loginDialog, "root": root, "bodyLoader": bodyLoader, "withSteam": completeProfileBody.withSteam,
                                    "linkSteam": false, "withOculus": completeProfileBody.withOculus, "linkOculus": false, "createOculus": true });
                                break;
                        }
                    }
                    onFocusChanged: {
                        root.text = "";
                        if (focus) {
                            root.isPassword = false;
                        }
                    }
                    Component.onCompleted: {
                        var userID = "";
                        if (completeProfileBody.withOculus) {
                            userID = loginDialog.oculusUserID();
                        }
                        usernameField.text = userID;
                    }
                }
                HifiControlsUit.TextField {
                    id: emailField
                    width: root.bannerWidth
                    height: completeProfileBody.textFieldHeight
                    anchors {
                        top: usernameField.bottom
                        topMargin: hifi.dimensions.contentSpacing.y
                    }
                    placeholderText: "Email"
                    font.pixelSize: completeProfileBody.textFieldFontSize
                    styleRenderType: Text.QtRendering
                    activeFocusOnPress: true
                    Keys.onPressed: {
                        switch (event.key) {
                            case Qt.Key_Tab:
                                event.accepted = true;
                                if (event.modifiers === Qt.ShiftModifier) {
                                    usernameField.focus = true;
                                } else {
                                    passwordField.focus = true;
                                }
                                break;
                            case Qt.Key_Backtab:
                                event.accepted = true;
                                usernameField.focus = true;
                                break;
                            case Qt.Key_Enter:
                            case Qt.Key_Return:
                                event.accepted = true;
                                loginDialog.createAccountFromOculus(emailField.text, usernameField.text, passwordField.text);
                                bodyLoader.setSource("LoggingInBody.qml", { "loginDialog": loginDialog, "root": root, "bodyLoader": bodyLoader, "withSteam": completeProfileBody.withSteam,
                                    "linkSteam": false, "withOculus": completeProfileBody.withOculus, "linkOculus": false, "createOculus": true });
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
                    height: completeProfileBody.textFieldHeight
                    placeholderText: "Password (optional)"
                    font.pixelSize: completeProfileBody.textFieldFontSize
                    styleRenderType: Text.QtRendering
                    activeFocusOnPress: true
                    echoMode: passwordFieldMouseArea.showPassword ? TextInput.Normal : TextInput.Password
                    anchors {
                        top: emailField.bottom
                        topMargin: hifi.dimensions.contentSpacing.y
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
                            loginDialog.createAccountFromOculus(emailField.text, usernameField.text, passwordField.text);
                            break;
                        }
                    }
                }
            }

            Item {
                id: buttons
                width: root.bannerWidth
                height: d.minHeightButton
                anchors {
                    top: parent.top
                    topMargin: (parent.height - additionalTextContainer.height + fields.height) / 2 - hifi.dimensions.contentSpacing.y
                    left: parent.left
                    leftMargin: (parent.width - root.bannerWidth) / 2
                }
                HifiControlsUit.Button {
                    id: cancelButton
                    anchors {
                        top: parent.top
                        left: parent.left
                    }
                    width: (parent.width - hifi.dimensions.contentSpacing.x) / 2
                    height: d.minHeightButton

                    text: qsTr("CANCEL")
                    color: hifi.buttons.noneBorderlessWhite

                    fontFamily: completeProfileBody.fontFamily
                    fontSize: completeProfileBody.fontSize
                    fontBold: completeProfileBody.fontBold
                    onClicked: {
                        bodyLoader.setSource("LinkAccountBody.qml", { "loginDialog": loginDialog, "root": root, "bodyLoader": bodyLoader });
                    }
                }
                HifiControlsUit.Button {
                    id: profileButton
                    anchors {
                        top: parent.top
                        right: parent.right
                    }
                    width: (parent.width - hifi.dimensions.contentSpacing.x) / 2
                    height: d.minHeightButton

                    text: completeProfileBody.withOculus ? qsTr("Sign Up") : qsTr("Create your profile")
                    color: hifi.buttons.blue

                    fontFamily: completeProfileBody.fontFamily
                    fontSize: completeProfileBody.fontSize
                    fontBold: completeProfileBody.fontBold
                    onClicked: {
                        loginErrorMessage.visible = false;
                        if (completeProfileBody.withOculus) {
                            loginDialog.createAccountFromOculus(emailField.text, usernameField.text, passwordField.text);
                            bodyLoader.setSource("LoggingInBody.qml", { "loginDialog": loginDialog, "root": root, "bodyLoader": bodyLoader, "withSteam": completeProfileBody.withSteam,
                                "linkSteam": false, "withOculus": completeProfileBody.withOculus, "linkOculus": false, "createOculus": true });
                        } else if (completeProfileBody.withSteam) {
                            loginDialog.createAccountFromSteam();
                        }
                    }
                }
            }

            Item {
                id: termsContainer
                width: parent.width
                height: termsTextMetrics.height
                anchors {
                    top: buttons.bottom
                    horizontalCenter: parent.horizontalCenter
                    topMargin: hifi.dimensions.contentSpacing.y
                    left: parent.left
                }
                TextMetrics {
                    id: termsTextMetrics
                    font: termsText.font
                    text: completeProfileBody.withOculus ? completeProfileBody.termsContainerOculusText : completeProfileBody.termsContainerText
                    Component.onCompleted: {
                        // with the link.
                        if (completeProfileBody.withOculus) {
                            termsText.text = qsTr("By signing up, you agree to <a href='https://highfidelity.com/terms'>High Fidelity's Terms of Service</a>")
                        } else {
                            termsText.text = qsTr("By creating this user profile, you agree to <a href='https://highfidelity.com/terms'>High Fidelity's Terms of Service</a>")
                        }
                    }
                }

                HifiStylesUit.InfoItem {
                    id: termsText
                    text: completeProfileBody.withOculus ? completeProfileBody.termsContainerOculusText : completeProfileBody.termsContainerText
                    font.family: completeProfileBody.fontFamily
                    font.pixelSize: completeProfileBody.fontSize
                    font.bold: completeProfileBody.fontBold
                    wrapMode: Text.WordWrap
                    color: hifi.colors.white
                    linkColor: hifi.colors.blueAccent
                    lineHeight: 1
                    lineHeightMode: Text.ProportionalHeight

                    onLinkActivated: loginDialog.openUrl(link);

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

            Item {
                id: additionalTextContainer
                width: parent.width
                height: additionalTextMetrics.height
                anchors {
                    top: termsContainer.bottom
                    horizontalCenter: parent.horizontalCenter
                    topMargin: 2 * hifi.dimensions.contentSpacing.y
                    left: parent.left
                }

                TextMetrics {
                    id: additionalTextMetrics
                    font: additionalText.font
                    text: "Already have a High Fidelity profile? Link to an existing profile here."
                }

                HifiStylesUit.ShortcutText {
                    id: additionalText
                    text: "<a href='https://fake.link'>Already have a High Fidelity profile? Link to an existing profile here.</a>"
                    width: root.bannerWidth;
                    font.family: completeProfileBody.fontFamily
                    font.pixelSize: completeProfileBody.fontSize
                    font.bold: completeProfileBody.fontBold
                    wrapMode: Text.NoWrap
                    lineHeight: 1
                    lineHeightMode: Text.ProportionalHeight
                    horizontalAlignment: Text.AlignHCenter
                    linkColor: hifi.colors.blueAccent

                    onLinkActivated: {
                        loginDialog.isLogIn = true;
                        bodyLoader.setSource("LinkAccountBody.qml", { "loginDialog": loginDialog, "root": root, "bodyLoader": bodyLoader, "errorString": "",
                            "withSteam": completeProfileBody.withSteam, "linkSteam": completeProfileBody.withSteam, "withOculus": completeProfileBody.withOculus,
                            "linkOculus": completeProfileBody.withOculus });
                    }
                    Component.onCompleted: {
                        if (additionalTextMetrics.width > root.bannerWidth) {
                            additionalText.wrapMode = Text.WordWrap;
                            additionalText.verticalAlignment = Text.AlignLeft;
                            additionalText.horizontalAlignment = Text.AlignLeft;
                            additionalTextContainer.height = (additionalTextMetrics.width / root.bannerWidth) * additionalTextMetrics.height;
                            additionalTextContainer.anchors.left = buttons.left;
                        } else {
                            additionalText.anchors.centerIn = additionalTextContainer;
                        }
                    }
                }
            }
        }
    }

    Connections {
        target: loginDialog
        onHandleCreateCompleted: {
            console.log("Create Succeeded");
            if (completeProfileBody.withSteam) {
                loginDialog.loginThroughSteam();
            }
            bodyLoader.setSource("LoggingInBody.qml", { "loginDialog": loginDialog, "root": root, "bodyLoader": bodyLoader, "withSteam": completeProfileBody.withSteam, "linkSteam": false,
                "withOculus": completeProfileBody.withOculus, "linkOculus": false });
        }
        onHandleCreateFailed: {
            console.log("Create Failed: " + error);
            bodyLoader.setSource("UsernameCollisionBody.qml", { "loginDialog": loginDialog, "root": root, "bodyLoader": bodyLoader, "withSteam": completeProfileBody.withSteam,
                "withOculus": completeProfileBody.withOculus });
        }
    }

    Component.onCompleted: {
        //but rise Tablet's one instead for Tablet interface
        if (root.isTablet || root.isOverlay) {
            root.keyboardEnabled = HMD.active;
            root.keyboardRaised = Qt.binding( function() { return keyboardRaised; })
        }
        d.resize();
        root.text = "";
        usernameField.forceActiveFocus();
    }
}
