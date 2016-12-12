//
//  LoginDialog.qml
//
//  Created by David Rowe on 3 Jun 2015
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import Hifi 1.0
import QtQuick 2.4

import "controls-uit"
import "styles-uit"
import "windows"

import "LoginDialog"

ModalWindow {
    id: root
    HifiConstants { id: hifi }
    objectName: "LoginDialog"
    implicitWidth: 520
    implicitHeight: 320
    destroyOnCloseButton: true
    destroyOnHidden: true
    visible: true

    property string iconText: ""
    property int iconSize: 50

    property string title: ""
    property int titleWidth: 0

    keyboardOverride: true  // Disable ModalWindow's keyboard.

    LoginDialog {
        id: loginDialog

        Loader {
            id: bodyLoader
            source: loginDialog.isSteamRunning() ? "LoginDialog/SignInBody.qml" : "LoginDialog/LinkAccountBody.qml"
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
                destroy()
                break

            case Qt.Key_Enter:
            case Qt.Key_Return:
                event.accepted = true
                break
        }
    }
}
