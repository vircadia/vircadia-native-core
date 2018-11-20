//
//  TabletLoginDialog.qml
//
//  Created by Vlad Stelmahovsky on 15 Mar 2017
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import Hifi 1.0
import QtQuick 2.5

import controlsUit 1.0
import stylesUit 1.0
import "../windows"

import "../LoginDialog"

TabletModalWindow {
    id: realRoot
    objectName: "LoginDialog"

    signal sendToScript(var message);
    property bool isHMD: false
    property bool gotoPreviousApp: false;
    color: hifi.colors.baseGray
    title: qsTr("Sign in to High Fidelity")
    property alias titleWidth: root.titleWidth
    property alias punctuationMode: root.punctuationMode

    //fake root for shared components expecting root here
    property var root: QtObject {
        id: root

        property bool keyboardEnabled: false
        property bool keyboardRaised: false
        property bool punctuationMode: false
        property bool isPassword: false
        property alias text: loginKeyboard.mirroredText

        readonly property bool isTablet: true

        property alias title: realRoot.title
        property real width: realRoot.width
        property real height: realRoot.height

        property int titleWidth: 0
        property string iconText: hifi.glyphs.avatar
        property int iconSize: 35

        property var pane: QtObject {
            property real width: root.width
            property real height: root.height
        }

        function tryDestroy() {
            canceled()
        }
    }

    //property int colorScheme: hifi.colorSchemes.dark

    MouseArea {
        width: realRoot.width
        height: realRoot.height
    }

    property bool keyboardOverride: true

    property var items;
    property string label: ""

    //onTitleWidthChanged: d.resize();

    //onKeyboardRaisedChanged: d.resize();

    signal canceled();

    property alias bodyLoader: bodyLoader
    property alias loginDialog: loginDialog
    property alias hifi: hifi

    HifiConstants { id: hifi }

    onCanceled: {
        if (bodyLoader.active === true) {
            //bodyLoader.active = false
        }
        if (gotoPreviousApp) {
            var tablet = Tablet.getTablet("com.highfidelity.interface.tablet.system");
            tablet.returnToPreviousApp();
        } else {
            Tablet.getTablet("com.highfidelity.interface.tablet.system").gotoHomeScreen();
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

    TabletModalFrame {
        id: mfRoot

        width: root.width
        height: root.height + frameMarginTop + hifi.dimensions.contentMargin.x

        anchors {
            horizontalCenter: parent.horizontalCenter
            verticalCenter: parent.verticalCenter
            verticalCenterOffset: -loginKeyboard.height / 2
        }

        LoginDialog {
            id: loginDialog

            anchors {
                fill: parent
                topMargin: parent.frameMarginTop
                leftMargin: hifi.dimensions.contentMargin.x
                rightMargin: hifi.dimensions.contentMargin.x
                horizontalCenter: parent.horizontalCenter
            }

            Loader {
                id: bodyLoader
                anchors.fill: parent
                anchors.horizontalCenter: parent.horizontalCenter
                source: loginDialog.isSteamRunning() ? "../LoginDialog/SignInBody.qml" : "../LoginDialog/LinkAccountBody.qml"
            }
        }
    }

    Component.onDestruction: {
        loginKeyboard.raised = false;
        KeyboardScriptingInterface.raised = false;
    }

    Component.onCompleted: {
        keyboardTimer.start();
    }

    Keyboard {
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
                destroy()
                break

            case Qt.Key_Enter:
            case Qt.Key_Return:
                event.accepted = true
                break
        }
    }
}
