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

import "qrc:///qml//controls-uit" as HifiControlsUit
import "qrc:///qml//styles-uit" as HifiStylesUit
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


    onKeyboardRaisedChanged: d.resize();

    QtObject {
        id: d
        readonly property int minWidth: 480
        property int maxWidth: root.isTablet ? 1280 : Window.innerWidth
        readonly property int minHeight: 120
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
        Text {
            id: flavorText
            text: qsTr("BE ANYWHERE, WITH ANYONE RIGHT NOW")
            width: 0.48 * parent.width
            anchors.centerIn: parent
            anchors {
                top: bannerContainer.bottom
                topMargin: 0.1 * parent.height
            }
            wrapMode: Text.WordWrap
            lineHeight: 1
            color: "white"
            font.family: "Raleway"
            font.pixelSize: 55
            font.bold: true
            lineHeightMode: Text.ProportionalHeight
            horizontalAlignment: Text.AlignHCenter
        }

        HifiControlsUit.Button {
            id: signUpButton
            text: qsTr("Sign Up")
            width: 0.17 * parent.width
            height: 0.068 * parent.height
            color: hifi.buttons.blue
            fontSize: 30
            anchors {
                top: flavorText.bottom
                topMargin: 0.1 * parent.height
                left: parent.left
                    leftMargin: (parent.width - signUpButton.width) / 2
            }

            onClicked: {
                bodyLoader.setSource("SignUpBody.qml")
                if (!linkAccountBody.isTablet) {
                    bodyLoader.item.width = root.pane.width
                    bodyLoader.item.height = root.pane.height
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
            height: root.pane.height
            width: root.pane.width
            opacity: 0.7
            color: "black"
        }

        HifiControlsUit.Button {
            id: loginButton
            width: signUpButton.width
            height: signUpButton.height
            text: qsTr("Log In")
            fontSize: signUpButton.fontSize
            // background: Rectangle {
            //     radius: hifi.buttons.radius
            //
            // }
            anchors {
                top: parent.top
                topMargin: 0.245 * parent.height
                left: parent.left
                leftMargin: (parent.width - loginButton.width) / 2
            }

            onClicked: {
                bodyLoader.setSource("SignInBody.qml")
                if (!linkAccountBody.isTablet) {
                    loginDialog.bodyLoader.item.width = root.pane.width
                    loginDialog.bodyLoader.item.height = root.pane.height
                }
            }
        }
        HifiControlsUit.Button {
            id: steamLoginButton
            width: signUpButton.width
            height: signUpButton.height
            text: qsTr("Link Account")
            fontSize: signUpButton.fontSize
            color: hifi.buttons.black
            anchors {
                top: loginButton.bottom
                topMargin: 0.04 * parent.height
                left: parent.left
                leftMargin: (parent.width - steamLoginButton.width) / 2
            }

            onClicked: {
                if (loginDialog.isSteamRunning()) {
                    loginDialog.linkSteam();
                }
                if (!linkAccountBody.isTablet) {
                    bodyLoader.item.width = root.pane.width
                    bodyLoader.item.height = root.pane.height
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
                font.family: "Raleway"
                font.pixelSize: 20
                font.bold: true
                lineHeightMode: Text.ProportionalHeight
                // horizontalAlignment: Text.AlignHCenter
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
        root.title = qsTr("Sign Into High Fidelity")
        root.iconText = "<"

        //dont rise local keyboard
        keyboardEnabled = !root.isTablet && HMD.active;
        //but rise Tablet's one instead for Tablet interface
        if (root.isTablet) {
            root.keyboardEnabled = HMD.active;
            root.keyboardRaised = Qt.binding( function() { return keyboardRaised; })
        }
        d.resize();

        // if (failAfterSignUp) {
        //     mainTextContainer.text = "Account created successfully."
        //     flavorText.visible = true
        //     mainTextContainer.visible = true
        // }

    }

    Connections {
        target: loginDialog
        onHandleLoginCompleted: {
            console.log("Login Succeeded, linking steam account")
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
                bodyLoader.setSource("WelcomeBody.qml", { "welcomeBack" : true })
                bodyLoader.item.width = root.pane.width
                bodyLoader.item.height = root.pane.height
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
            flavorText.visible = true
            mainTextContainer.visible = true
            toggleLoading(false)
        }
        onHandleLinkCompleted: {
            console.log("Link Succeeded")

            bodyLoader.setSource("WelcomeBody.qml", { "welcomeBack" : true })
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
            return
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
