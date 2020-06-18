//
//  LoginDialog.qml
//
//  Created by Wayne Chen
//  Copyright 2018 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import Hifi 1.0
import QtQuick 2.4

import controlsUit 1.0 as HifiControlsUit
import stylesUit 1.0 as HifiStylesUit

import "LoginDialog"

FocusScope {
    id: root
    HifiStylesUit.HifiConstants { id: hifi }
    objectName: "LoginDialog"
    visible: true

    anchors.fill: parent

    readonly property bool isTablet: false
    readonly property bool isOverlay: true
    property bool keyboardEnabled: false
    property bool keyboardRaised: false
    property bool punctuationMode: false
    property bool isPassword: false
    property string iconText: ""
    property int iconSize: 50

    property string title: ""
    property int titleWidth: 0
    property alias text: loginKeyboard.mirroredText
    property alias bannerWidth: banner.width
    property alias bannerHeight: banner.height

    function tryDestroy() {
        root.destroy()
    }

    LoginDialog {
        id: loginDialog
        anchors.fill: parent
        Loader {
            id: bodyLoader
            anchors.fill: parent
        }
    }

    Image {
        z: -10
        id: loginDialogBackground
        fillMode: Image.PreserveAspectCrop
        source: "LoginDialog/images/background.png"
        anchors.fill: parent
    }

    Rectangle {
        z: -6
        id: opaqueRect
        height: parent.height
        width: parent.width
        opacity: 0.65
        color: "black"
    }

    Item {
        z: -5
        id: bannerContainer
        width: parent.width
        height: banner.height
        anchors {
            top: parent.top
            topMargin: 0.18 * parent.height
        }
        Image {
            id: banner
            anchors.centerIn: parent
            sourceSize.width: 500
            sourceSize.height: 91
            source: "../images/vircadia-banner.svg"
            horizontalAlignment: Image.AlignHCenter
        }
    }

    Timer {
        id: keyboardTimer
        repeat: false
        interval: 200

        onTriggered: {
            if (MenuInterface.isOptionChecked("Use 3D Keyboard")) {
                KeyboardScriptingInterface.raised = true;
            }
        }
    }

    HifiControlsUit.Keyboard {
        id: loginKeyboard
        raised: root.keyboardEnabled && root.keyboardRaised
        numeric: root.punctuationMode
        password: root.isPassword
        anchors {
            left: parent.left
            right: parent.right
            bottom: parent.bottom
        }
    }

    Keys.onPressed: {
        if (!visible) {
            return
        }

        if (event.modifiers === Qt.ControlModifier)
            switch (event.key) {
            case Qt.Key_A:
                event.accepted = true
                detailedText.selectAll()
                break
            case Qt.Key_C:
                event.accepted = true
                detailedText.copy()
                break
            case Qt.Key_Period:
                if (Qt.platform.os === "osx") {
                    event.accepted = true
                    content.reject()
                }
                break
        } else switch (event.key) {
            case Qt.Key_Escape:
            case Qt.Key_Back:
                event.accepted = true
                break

            case Qt.Key_Enter:
            case Qt.Key_Return:
                event.accepted = true
                break
        }
    }

    Component.onDestruction: {
        loginKeyboard.raised = false;
    }

    Component.onCompleted: {
        keyboardTimer.start();
        bodyLoader.setSource("LoginDialog/LinkAccountBody.qml", { "loginDialog": loginDialog, "root": root, "bodyLoader": bodyLoader, "linkSteam": false, "linkOculus": false });
    }
}
