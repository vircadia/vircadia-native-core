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

Item {
    id: completeProfileBody
    clip: true
    width: root.pane.width
    height: root.pane.height
    readonly property string termsContainerText: qsTr("By creating this user profile, you agree to High Fidelity's Terms of Service")
    readonly property string fontFamily: "Cairo"
    readonly property int fontSize: 24
    readonly property bool fontBold: true

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
            if (root.isTablet === false) {
                var targetWidth = Math.max(Math.max(titleWidth, Math.max(additionalTextContainer.contentWidth,
                                                                termsContainer.contentWidth)), mainContainer.width)
                parent.width = root.width = Math.max(d.minWidth, Math.min(d.maxWidth, targetWidth))
            }
            var targetHeight = Math.max(5 * hifi.dimensions.contentSpacing.y + buttons.height + additionalTextContainer.height + termsContainer.height, d.maxHeight)
            parent.height = root.height = Math.max(d.minHeight, Math.min(d.maxHeight, targetHeight))
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

    function hideContents(hide) {
        additionalTextContainer.visible = !hide;
        termsContainer.visible = !hide;
        buttons.visible = !hide;
    }

    function loginSuccess(success) {
        loginErrorMessage.visible = true;
        loggedInGlyph.visible = success;
        loginErrorMessage.text = success ? "You are now logged into Steam!" : "Error logging into Steam."
        loginErrorMessage.color = success ? "white" : "red";
        loginErrorMessage.font.pixelSize = success ? 24 : 12;
        loginErrorMessage.anchors.leftMargin = (mainContainer.width - loginErrorMessageTextMetrics.width) / 2;
        completeProfileBody.hideContents(success);
        if (success) {
            successTimer.start();
        }
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
            id: contentItem
            anchors.fill: parent

            TextMetrics {
                id: loginErrorMessageTextMetrics
                font: loginErrorMessage.font
                text: loginErrorMessage.text
            }
            Text {
                id: loginErrorMessage
                anchors.top: parent.top;
                // above buttons.
                anchors.topMargin: (parent.height - additionalTextContainer.height) / 2 - hifi.dimensions.contentSpacing.y - buttons.height
                anchors.left: parent.left;
                color: "red";
                font.family: "Cairo"
                font.pixelSize: 12
                text: ""
                visible: true
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
                anchors.top: loginErrorMessage.bottom
                anchors.topMargin: 2 * hifi.dimensions.contentSpacing.y
                // Alignment
                horizontalAlignment: Text.AlignHCenter;
                verticalAlignment: Text.AlignVCenter;
                visible: false;

            }
            Row {
                id: buttons
                anchors {
                    top: parent.top
                    topMargin: (parent.height - additionalTextContainer.height) / 2 - hifi.dimensions.contentSpacing.y
                    horizontalCenter: parent.horizontalCenter
                    margins: 0
                }
                spacing: hifi.dimensions.contentSpacing.x
                onHeightChanged: d.resize(); onWidthChanged: d.resize();

                HifiControlsUit.Button {
                    id: profileButton
                    anchors.verticalCenter: parent.verticalCenter
                    width: 256

                    text: qsTr("Create your profile")
                    color: hifi.buttons.blue

                    fontFamily: completeProfileBody.fontFamily
                    fontSize: completeProfileBody.fontSize
                    fontBold: completeProfileBody.fontBold
                    onClicked: {
                        loginErrorMessage.visible = false;
                        loginDialog.createAccountFromStream()
                    }
                }

                HifiControlsUit.Button {
                    id: cancelButton
                    anchors.verticalCenter: parent.verticalCenter
                    text: qsTr("Cancel")
                    fontFamily: completeProfileBody.fontFamily
                    fontSize: completeProfileBody.fontSize
                    fontBold: completeProfileBody.fontBold
                    onClicked: {
                        bodyLoader.setSource("LinkAccountBody.qml", { "loginDialog": loginDialog, "root": root, "bodyLoader": bodyLoader });
                    }
                }
            }

            HifiStylesUit.ShortcutText {
                id: additionalTextContainer
                anchors {
                    top: buttons.bottom
                    horizontalCenter: parent.horizontalCenter
                    margins: 0
                    topMargin: hifi.dimensions.contentSpacing.y
                }

                text: "<a href='https://fake.link'>Already have a High Fidelity profile? Link to an existing profile here.</a>"

                font.family: completeProfileBody.fontFamily
                font.pixelSize: 14
                font.bold: completeProfileBody.fontBold
                wrapMode: Text.WordWrap
                lineHeight: 2
                lineHeightMode: Text.ProportionalHeight
                horizontalAlignment: Text.AlignHCenter

                onLinkActivated: {
                    loginDialog.atSignIn = true;
                    loginDialog.isLogIn = true;
                    bodyLoader.setSource("SignInBody.qml", { "loginDialog": loginDialog, "root": root, "bodyLoader": bodyLoader });
                }
            }

            TextMetrics {
                id: termsContainerTextMetrics
                font: termsContainer.font
                text: completeProfileBody.termsContainerText
                Component.onCompleted: {
                    // with the link.
                    termsContainer.text = qsTr("By creating this user profile, you agree to <a href='https://highfidelity.com/terms'>High Fidelity's Terms of Service</a>")
                }
            }

            HifiStylesUit.InfoItem {
                id: termsContainer
                anchors {
                    top: additionalTextContainer.bottom
                    margins: 0
                    topMargin: 2 * hifi.dimensions.contentSpacing.y
                    horizontalCenter: parent.horizontalCenter
                    left: parent.left
                    leftMargin: (parent.width - termsContainerTextMetrics.width) / 2
                }

                text: completeProfileBody.termsContainerText
                font.family: completeProfileBody.fontFamily
                font.pixelSize: 14
                font.bold: completeProfileBody.fontBold
                wrapMode: Text.WordWrap
                color: hifi.colors.baseGrayHighlight
                lineHeight: 1
                lineHeightMode: Text.ProportionalHeight

                onLinkActivated: loginDialog.openUrl(link)
            }
        }
    }

    Component.onCompleted: {
        d.resize();
    }

    Connections {
        target: loginDialog
        onHandleCreateCompleted: {
            console.log("Create Succeeded")

            loginDialog.loginThroughSteam()
        }
        onHandleCreateFailed: {
            console.log("Create Failed: " + error)
            var poppedUp = Settings.getValue("loginDialogPoppedUp", false);
            if (poppedUp) {
                console.log("[ENCOURAGELOGINDIALOG]: failed creating an account")
                var data = {
                    "action": "user failed creating an account"
                };
                UserActivityLogger.logAction("encourageLoginDialog", data);
            }
            bodyLoader.setSource("UsernameCollisionBody.qml", { "loginDialog": loginDialog, "root": root, "bodyLoader": bodyLoader });
        }
        onHandleLoginCompleted: {
            console.log("Login Succeeded")
            loginSuccess(true)
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
            }
            loginSuccess(false)
        }
    }
}
