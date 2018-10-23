//
//  SignInBody.qml
//
//  Created by Clement on 7/18/16
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import Hifi 1.0
import QtQuick 2.7
import QtQuick.Controls.Styles 1.4 as OriginalStyles

import controlsUit 1.0 as HifiControlsUit
import stylesUit 1.0 as HifiStylesUit

Item {
    id: signInBody
    clip: true
    height: root.pane.height
    width: root.pane.width

    property bool required: false

    function login() {
        bodyLoader.setSource("LoggingInBody.qml");
        if (!root.isTablet) {
            bodyLoader.item.width = root.pane.width;
            bodyLoader.item.height = root.pane.height;
        }
        loginDialog.login(usernameField.text, passwordField.text);
    }

    function cancel() {
        bodyLoader.setSource("LinkAccountBody.qml");
        if (!root.isTablet) {
            bodyLoader.item.width = root.pane.width;
            bodyLoader.item.height = root.pane.height;
        }
    }

    QtObject {
        id: d
        readonly property int minWidth: 480
        property int maxWidth: root.isTablet ? 1280 : Window.innerWidth
        readonly property int minHeight: 120
        property int maxHeight: root.isTablet ? 720 : Window.innerHeight

        function resize() {
            maxWidth = root.isTablet ? 1280 : Window.innerWidth;
            maxHeight = root.isTablet ? 720 : Window.innerHeight;
            var targetWidth = Math.max(titleWidth, form.contentWidth)
            var targetHeight = 4 * hifi.dimensions.contentSpacing.y + form.height

            parent.width = root.width = Math.max(d.minWidth, Math.min(d.maxWidth, targetWidth))
            parent.height = root.height = Math.max(d.minHeight, Math.min(d.maxHeight, targetHeight))
        }
    }
    
    Item {
        width: parent.width
        height: parent.height
        Rectangle {
            width: parent.width
            height: parent.height
            opacity: 0.7
            color: "black"
        }
        Item {
            id: bannerContainer
            width: parent.width
            height: banner.height
            anchors {
                top: parent.top
                topMargin: 0.17 * parent.height
            }
            Image {
                id: banner
                anchors.centerIn: parent
                source: "../../images/high-fidelity-banner.svg"
                horizontalAlignment: Image.AlignHCenter
            }
        }
        Column {
            id: form
            width: parent.width
            onHeightChanged: d.resize(); onWidthChanged: d.resize();

            anchors {
                top: bannerContainer.bottom
                topMargin: 1.5 * hifi.dimensions.contentSpacing.y
            }
            spacing: 2 * hifi.dimensions.contentSpacing.y

            HifiControlsUit.TextField {
                id: usernameField
                width: 0.254 * parent.width
                text: Settings.getValue("wallet/savedUsername", "");
                focus: true
                placeholderText: "Username or Email"
                activeFocusOnPress: true
                onHeightChanged: d.resize(); onWidthChanged: d.resize();

                onFocusChanged: {
                    root.text = "";
                }
                Component.onCompleted: {
                    var savedUsername = Settings.getValue("wallet/savedUsername", "");
                    usernameField.text = savedUsername === "Unknown user" ? "" : savedUsername;
                }
            }
            HifiControlsUit.TextField {
                id: passwordField
                width: 0.254 * parent.width
                placeholderText: "Password"
                activeFocusOnPress: true
                echoMode: passwordFieldMouseArea.showPassword ? TextInput.Normal : TextInput.Password
                onHeightChanged: d.resize(); onWidthChanged: d.resize();

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

                Keys.onReturnPressed: signInBody.login()
            }
        }
    }

    // Row {
    //     id: buttons
    //     anchors {
    //         top: mainTextContainer.bottom
    //         horizontalCenter: parent.horizontalCenter
    //         margins: 0
    //         topMargin: 2 * hifi.dimensions.contentSpacing.y
    //     }
    //     spacing: hifi.dimensions.contentSpacing.x
    //     onHeightChanged: d.resize(); onWidthChanged: d.resize();
    //
    //     Button {
    //         anchors.verticalCenter: parent.verticalCenter
    //
    //         width: undefined // invalidate so that the image's size sets the width
    //         height: undefined // invalidate so that the image's size sets the height
    //         focus: true
    //
    //         background: Image {
    //             id: buttonImage
    //             source: "../../images/steam-sign-in.png"
    //         }
    //         onClicked: signInBody.login()
    //     }
    //     Button {
    //         anchors.verticalCenter: parent.verticalCenter
    //
    //         text: qsTr("Cancel");
    //
    //         onClicked: signInBody.cancel()
    //     }
    // }

    // Component.onCompleted: {
    //     root.title = required ? qsTr("Sign In Required")
    //                           : qsTr("Sign In")
    //     root.iconText = ""
    //     d.resize();
    // }

    Connections {
        target: loginDialog
        onHandleLoginCompleted: {
            console.log("Login Succeeded")

            bodyLoader.setSource("WelcomeBody.qml", { "welcomeBack" : true })
            bodyLoader.item.width = root.pane.width
            bodyLoader.item.height = root.pane.height
        }
        onHandleLoginFailed: {
            console.log("Login Failed")

            bodyLoader.source = "CompleteProfileBody.qml"
            if (!root.isTablet) {
                bodyLoader.item.width = root.pane.width
                bodyLoader.item.height = root.pane.height
            }
        }
    }
}
