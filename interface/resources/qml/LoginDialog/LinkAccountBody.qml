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

import "qrc:////qml//controls-uit" as HifiControlsUit
import "qrc:////qml//styles-uit" as HifiStylesUit

import TabletScriptingInterface 1.0

Item {
    id: linkAccountBody
    clip: true
    height: root.height
    width: root.width
    property bool failAfterSignUp: false
    property string fontFamily: "Cairo"
    property int fontSize: 24
    property bool fontBold: true

    property bool withSteam: false

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

    function toggleSignIn(isLogIn) {
        // going to/from sign in/up dialog.
        loginDialog.isLogIn = isLogIn;
        if (linkAccountBody.withSteam) {
            loginDialog.loginThroughSteam();
            bodyLoader.setSource("LoggingInBody.qml", { "loginDialog": loginDialog, "root": root, "bodyLoader": bodyLoader,
                "withSteam": linkAccountBody.withSteam, "fromBody": "LinkAccountBody" });
        } else {
            bodyLoader.setSource("SignInBody.qml", { "loginDialog": loginDialog, "root": root, "bodyLoader": bodyLoader, "errorString": "" });
        }
    }

    Item {
        id: topContainer
        width: root.width
        height: 0.6 * root.height
        onHeightChanged: d.resize(); onWidthChanged: d.resize();
        anchors {
            top: parent.top
            left: parent.left
            right: parent.right
        }

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
                font.pixelSize: !root.isTablet ? 2 * linkAccountBody.fontSize : linkAccountBody.fontSize
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
                    linkAccountBody.withSteam = false;
                    toggleSignIn(false);
                }
            }
        }
    }
    Item {
        id: bottomContainer
        width: root.width
        height: 0.4 * root.height
        onHeightChanged: d.resize(); onWidthChanged: d.resize();
        anchors {
            top: topContainer.bottom
            left: parent.left
            right: parent.right
        }

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
                    linkAccountBody.withSteam = false;
                    toggleSignIn(true);
                }
            }
            TextMetrics {
                id: steamLoginButtonTextMetrics
                font: dismissText.font
                text: qsTr("STEAM LOG IN")
            }
            HifiControlsUit.Button {
                id: steamLoginButton;
                // textWidth + size of glyph + rightMargin
                width: Math.max(d.minWidthButton, steamLoginButtonTextMetrics.width + 34 + buttonGlyphRightMargin + 2 * hifi.dimensions.contentSpacing.x);
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
                    toggleSignIn(true);
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
            visible: !root.isTablet && !HMD.active
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
    Component.onDestruction: {
        if (loginDialog.getLoginDialogPoppedUp() && root.isTablet) {
            // it popped up and was clicked with the X
            console.log("[ENCOURAGELOGINDIALOG]: user closed login screen")
            var data = {
                "action": "user dismissed login screen"
            };
            UserActivityLogger.logAction("encourageLoginDialog", data);
            loginDialog.dismissLoginDialog();
        }
    }
}
