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
import "controls"
import "styles"
import "windows"

ScrollingWindow {
    id: root
    HifiConstants { id: hifi }
    objectName: "LoginDialog"
    height: loginDialog.implicitHeight
    width: loginDialog.implicitWidth
    // FIXME make movable
    anchors.centerIn: parent
    destroyOnHidden: false
    hideBackground: true
    shown: false

    LoginDialog {
        id: loginDialog
        implicitWidth: backgroundRectangle.width
        implicitHeight: backgroundRectangle.height
        readonly property int inputWidth: 500
        readonly property int inputHeight: 60
        readonly property int borderWidth: 30
        readonly property int closeMargin: 16
        readonly property real tan30: 0.577  // tan(30°)
        readonly property int inputSpacing: 16

        Rectangle {
            id: backgroundRectangle
            width: loginDialog.inputWidth + loginDialog.borderWidth * 2
            height: loginDialog.inputHeight * 6 + loginDialog.closeMargin * 2
            radius: loginDialog.closeMargin * 2
            color: "#2c86b1"
            opacity: 0.85
        }

        Column {
            id: mainContent
            width: loginDialog.inputWidth
            spacing: loginDialog.inputSpacing
            anchors {
                horizontalCenter: parent.horizontalCenter
                verticalCenter: parent.verticalCenter
            }

            Item {
                // Offset content down a little
                width: loginDialog.inputWidth
                height: loginDialog.closeMargin
            }

            Rectangle {
                width: loginDialog.inputWidth
                height: loginDialog.inputHeight
                radius: height / 2
                color: "#ebebeb"

                Image {
                    source: "../images/login-username.svg"
                    width: loginDialog.inputHeight * 0.65
                    height: width
                    sourceSize: Qt.size(width, height);
                    anchors {
                        verticalCenter: parent.verticalCenter
                        left: parent.left
                        leftMargin: loginDialog.inputHeight / 4
                    }
                }

                TextInput {
                    id: username
                    anchors {
                        fill: parent
                        leftMargin: loginDialog.inputHeight
                        rightMargin: loginDialog.inputHeight / 2
                    }

                    helperText: "username or email"
                    color: hifi.colors.text

                    KeyNavigation.tab: password
                    KeyNavigation.backtab: password
                }
            }

            Rectangle {
                width: loginDialog.inputWidth
                height: loginDialog.inputHeight
                radius: height / 2
                color: "#ebebeb"

                Image {
                    source: "../images/login-password.svg"
                    width: loginDialog.inputHeight * 0.65
                    height: width
                    sourceSize: Qt.size(width, height);
                    anchors {
                        verticalCenter: parent.verticalCenter
                        left: parent.left
                        leftMargin: loginDialog.inputHeight / 4
                    }
                }

                TextInput {
                    id: password
                    anchors {
                        fill: parent
                        leftMargin: loginDialog.inputHeight
                        rightMargin: loginDialog.inputHeight / 2
                    }

                    helperText: "password"
                    echoMode: TextInput.Password
                    color: hifi.colors.text

                    KeyNavigation.tab: username
                    KeyNavigation.backtab: username
                    onFocusChanged: {
                        if (password.focus) {
                            password.selectAll()
                        }
                    }
                }
            }

            Item {
                width: loginDialog.inputWidth
                height: loginDialog.inputHeight / 2

                Text {
                    id: messageText

                    visible: loginDialog.statusText != "" && loginDialog.statusText != "Logging in..."

                    width: loginDialog.inputWidth
                    height: loginDialog.inputHeight / 2
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter

                    text: loginDialog.statusText
                    color: "white"
                }

                Row {
                    id: messageSpinner

                    visible: loginDialog.statusText == "Logging in..."
                    onVisibleChanged: visible ? messageSpinnerAnimation.restart() : messageSpinnerAnimation.stop()

                    spacing: 24
                    anchors {
                        verticalCenter: parent.verticalCenter
                        horizontalCenter: parent.horizontalCenter
                    }

                    Rectangle {
                        id: spinner1
                        width: 10
                        height: 10
                        color: "#ebebeb"
                        opacity: 0.05
                    }

                    Rectangle {
                        id: spinner2
                        width: 10
                        height: 10
                        color: "#ebebeb"
                        opacity: 0.05
                    }

                    Rectangle {
                        id: spinner3
                        width: 10
                        height: 10
                        color: "#ebebeb"
                        opacity: 0.05
                    }

                    SequentialAnimation {
                        id: messageSpinnerAnimation
                        running: messageSpinner.visible
                        loops: Animation.Infinite
                        NumberAnimation { target: spinner1; property: "opacity"; to: 1.0; duration: 1000 }
                        NumberAnimation { target: spinner2; property: "opacity"; to: 1.0; duration: 1000 }
                        NumberAnimation { target: spinner3; property: "opacity"; to: 1.0; duration: 1000 }
                        NumberAnimation { target: spinner1; property: "opacity"; to: 0.05; duration: 0 }
                        NumberAnimation { target: spinner2; property: "opacity"; to: 0.05; duration: 0 }
                        NumberAnimation { target: spinner3; property: "opacity"; to: 0.05; duration: 0 }
                    }
                }
            }

            Row {
                width: loginDialog.inputWidth
                height: loginDialog.inputHeight

                Rectangle {
                    width: loginDialog.inputWidth / 3
                    height: loginDialog.inputHeight
                    radius: height / 2
                    color: "#353535"

                    TextInput {
                        anchors.fill: parent
                        text: "Login"
                        color: "white"
                        horizontalAlignment: Text.AlignHCenter
                    }

                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            loginDialog.login(username.text, password.text)
                        }
                    }
                }

                Image {
                    source: "../images/steam-sign-in.png"
                    width: loginDialog.inputWidth / 3
                    height: loginDialog.inputHeight
                    anchors {
                        verticalCenter: parent.verticalCenter
                        right: parent.right
                        leftMargin: loginDialog.inputHeight / 4
                    }

                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: {
                            loginDialog.loginThroughSteam()
                        }
                    }
                }
            }

            Item {
                anchors { left: parent.left; right: parent.right; }
                height: loginDialog.inputHeight

                Image {
                    id: hifiIcon
                    source: "../images/hifi-logo-blackish.svg"
                    width: loginDialog.inputHeight
                    height: width
                    sourceSize: Qt.size(width, height);
                    anchors { verticalCenter: parent.verticalCenter; horizontalCenter: parent.horizontalCenter }
                }

                Text {
                    anchors { verticalCenter: parent.verticalCenter; right: hifiIcon.left; margins: loginDialog.inputSpacing }
                    text: "Password?"
                    scale: 0.8
                    font.underline: true
                    color: "#e0e0e0"
                    MouseArea {
                        anchors { fill: parent; margins: -loginDialog.inputSpacing / 2 }
                        cursorShape: Qt.PointingHandCursor
                        onClicked: loginDialog.openUrl(loginDialog.rootUrl + "/users/password/new")
                    }
                }

                Text {
                    anchors { verticalCenter: parent.verticalCenter; left: hifiIcon.right; margins: loginDialog.inputSpacing }
                    text: "Register"
                    scale: 0.8
                    font.underline: true
                    color: "#e0e0e0"

                    MouseArea {
                        anchors { fill: parent; margins: -loginDialog.inputSpacing / 2 }
                        cursorShape: Qt.PointingHandCursor
                        onClicked: loginDialog.openUrl(loginDialog.rootUrl + "/signup")
                    }
                }

            }
        }
    }

    onShownChanged: {
        if (!shown) {
            username.text = ""
            password.text = ""
            loginDialog.statusText = ""
        } else {
            username.forceActiveFocus()
        }
    }

    Keys.onPressed: {
        switch (event.key) {
            case Qt.Key_Escape:
            case Qt.Key_Back:
                root.shown = false;
                event.accepted = true;
                break;

            case Qt.Key_Enter:
            case Qt.Key_Return:
                if (username.activeFocus) {
                    event.accepted = true
                    password.forceActiveFocus()
                } else if (password.activeFocus) {
                    event.accepted = true
                    if (username.text == "") {
                        username.forceActiveFocus()
                    } else {
                        loginDialog.login(username.text, password.text)
                    }
                }
                break
        }
    }
}
