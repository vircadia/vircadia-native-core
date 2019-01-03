//
//  UsernameCollisionBody.qml
//
//  Created by Clement on 7/18/16
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import Hifi 1.0
import QtQuick 2.4
import QtQuick.Controls 1.4

import controlsUit 1.0 as HifiControlsUit
import stylesUit 1.0 as HifiStylesUit
import TabletScriptingInterface 1.0

Item {
    id: usernameCollisionBody
    clip: true
    width: root.width
    height: root.height
    readonly property string fontFamily: "Raleway"
    readonly property int fontSize: 15
    readonly property int textFieldFontSize: 18
    readonly property bool fontBold: true

    function create() {
        mainTextContainer.visible = false
        loginDialog.createAccountFromSteam(textField.text);
    }

    property bool keyboardEnabled: false
    property bool keyboardRaised: false
    property bool punctuationMode: false

    onKeyboardRaisedChanged: d.resize();

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
            var targetHeight =  mainTextContainer.height +
                                hifi.dimensions.contentSpacing.y + textField.height +
                                hifi.dimensions.contentSpacing.y + buttons.height;

            parent.width = root.width = Math.max(d.minWidth, Math.min(d.maxWidth, targetWidth))
            parent.height = root.height = Math.max(Math.max(d.minHeight, Math.min(d.maxHeight, targetHeight)), mainContainer.height + hifi.dimensions.contentSpacing.y);
        }
    }

    Item {
        id: mainContainer
        width: root.width
        height: root.height
        onHeightChanged: d.resize(); onWidthChanged: d.resize();
        anchors.fill: parent

        TextMetrics {
            id: mainTextContainerTextMetrics
            font: mainTextContainer.font
            text: mainTextContainer.text
        }

        Text {
            id: mainTextContainer
            anchors {
                top: parent.top
                left: parent.left
                leftMargin: (parent.width - mainTextContainer.width) / 2
                topMargin: parent.height/2 - buttons.anchors.topMargin - textField.height - mainTextContainerTextMetrics.height
            }

            font.family: usernameCollisionBody.fontFamily
            font.pixelSize: usernameCollisionBody.fontSize
            font.bold: usernameCollisionBody.fontBold
            text: qsTr("Your Steam username is not available.");
            wrapMode: Text.WordWrap
            color: hifi.colors.redAccent
            lineHeight: 1
            lineHeightMode: Text.ProportionalHeight
            horizontalAlignment: Text.AlignHCenter
        }


        HifiControlsUit.TextField {
            id: textField
            anchors {
                top: mainTextContainer.bottom
                left: parent.left
                leftMargin: (parent.width - width) / 2
                topMargin: hifi.dimensions.contentSpacing.y
            }
            focus: true
            font.family: "Fira Sans"
            font.pixelSize: usernameCollisionBody.textFieldFontSize
            styleRenderType: Text.QtRendering
            width: root.bannerWidth

            placeholderText: "Choose your own"

            onFocusChanged: {
                root.text = "";
                if (focus) {
                    root.isPassword = false;
                }
            }

            Keys.onPressed: {
                if (!visible) {
                    return;
                }

                switch (event.key) {
                    case Qt.Key_Enter:
                    case Qt.Key_Return:
                        event.accepted = true;
                        usernameCollisionBody.create();
                        break;
                }
            }
        }

        Item {
            id: buttons
            width: root.bannerWidth
            height: d.minHeightButton
            anchors {
                top: textField.bottom
                topMargin: hifi.dimensions.contentSpacing.y
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

                fontFamily: usernameCollisionBody.fontFamily
                fontSize: usernameCollisionBody.fontSize
                fontBold: usernameCollisionBody.fontBold
                onClicked: {
                    bodyLoader.setSource("CompleteProfileBody.qml", { "loginDialog": loginDialog, "root": root, "bodyLoader": bodyLoader, "errorString": "" });
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

                text: qsTr("Create your profile")
                color: hifi.buttons.blue

                fontFamily: usernameCollisionBody.fontFamily
                fontSize: usernameCollisionBody.fontSize
                fontBold: usernameCollisionBody.fontBold
                onClicked: {
                    usernameCollisionBody.create();
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
    }

    Connections {
        target: loginDialog
        onHandleCreateCompleted: {
            console.log("Create Succeeded");
            loginDialog.loginThroughSteam();
            bodyLoader.setSource("LoggingInBody.qml", { "loginDialog": loginDialog, "root": root, "bodyLoader": bodyLoader, "withSteam": true, "linkSteam": false })
        }
        onHandleCreateFailed: {
            console.log("Create Failed: " + error)

            mainTextContainer.visible = true
            mainTextContainer.text = "\"" + textField.text + qsTr("\" is invalid or already taken.");
        }
        onHandleLoginCompleted: {
            console.log("Login Succeeded");
            if (loginDialog.getLoginDialogPoppedUp()) {
                loginDialog.dismissLoginDialog();
            }
            root.tryDestroy();
        }

        onHandleLoginFailed: {
            console.log("Login Failed")
            mainTextContainer.text = "Login Failed";
        }


        onFocusEnabled: {
            if (!usernameCollisionBody.lostFocus) {
                Qt.callLater(function() {
                    textField.forceActiveFocus();
                });
            }
        }
        onFocusDisabled: {
            usernameCollisionBody.lostFocus = !root.isTablet && !root.isOverlay;
            if (nusernameCollisionBody.lostFocus) {
                Qt.callLater(function() {
                    textField.focus = false;
                });
            }
        }
    }
}
