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

import "qrc:////qml//controls-uit" as HifiControlsUit
import "qrc:////qml//styles-uit" as HifiStylesUit
import "windows" as Windows

import "LoginDialog"

Rectangle {
    id: root
    HifiStylesUit.HifiConstants { id: hifi }
    objectName: "LoginDialog"
    // implicitWidth: 520
    // implicitHeight: 320
    // destroyOnCloseButton: true
    // destroyOnHidden: true
    visible: true
    // frame: Item {}

    anchors.fill: parent

    readonly property bool isTablet: false

    property string iconText: ""
    property int iconSize: 50

    property string title: ""
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
        bodyLoader.setSource("LoginDialog/LinkAccountBody.qml", { "loginDialog": loginDialog, "root": root, "bodyLoader": bodyLoader });
    }

}
