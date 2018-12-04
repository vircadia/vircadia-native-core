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
    property bool shown: true
    visible: shown

    anchors.fill: parent

    readonly property bool isTablet: false
    readonly property bool isOverlay: false

    property string iconText: ""
    property int iconSize: 50
    property bool keyboardEnabled: false
    property bool keyboardRaised: false
    property bool punctuationMode: false
    property bool isPassword: false
    property string title: ""
    property string text: ""
    property int titleWidth: 0

    function tryDestroy() {
        root.destroy()
    }

    LoginDialog {
        id: loginDialog

        Loader {
            id: bodyLoader
        }
    }

    Image {
        id: loginDialogBackground
        source: "LoginDialog/background.png"
        anchors.fill: parent
        z: -2
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

    Component.onCompleted: {
        bodyLoader.setSource("LoginDialog/LinkAccountBody.qml", { "loginDialog": loginDialog, "root": root, "bodyLoader": bodyLoader, "linkSteam": false });
    }
}
