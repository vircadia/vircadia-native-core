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

Dialog {
    id: root
    HifiConstants { id: hifi }

    objectName: "LoginDialog"

    property bool destroyOnInvisible: false

    implicitWidth: loginDialog.implicitWidth
    implicitHeight: loginDialog.implicitHeight

    x: parent ? parent.width / 2 - width / 2 : 0
    y: parent ? parent.height / 2 - height / 2 : 0
    property int maximumX: parent ? parent.width - width : 0
    property int maximumY: parent ? parent.height - height : 0

    function isCircular() {
        return loginDialog.dialogFormat == "circular"
    }

    LoginDialog {
        id: loginDialog

        implicitWidth: isCircular() ? circularBackground.width : rectangularBackground.width
        implicitHeight: isCircular() ? circularBackground.height : rectangularBackground.height

        property int inputWidth: 500
        property int inputHeight: 60
        property int inputSpacing: isCircular() ? 24 : 16
        property int borderWidth: 30
        property int closeMargin: 16

        Image {
            id: circularBackground
            visible: isCircular()
            source: "../images/login-circle.svg"
            width: loginDialog.inputWidth * 1.2
            height: width
        }

        Image {
            id: rectangularBackground
            visible: !isCircular()
            source: "../images/login-rectangle.svg"
            width: loginDialog.inputWidth + loginDialog.borderWidth * 2
            height: loginDialog.inputHeight * 6
        }

        Image {
            id: closeIcon
            visible: !isCircular()
            source: "../images/login-close.svg"
            width: 20
            height: 20
            anchors {
                top: rectangularBackground.top
                right: rectangularBackground.right
                topMargin: loginDialog.closeMargin
                rightMargin: loginDialog.closeMargin
            }

            MouseArea {
                anchors.fill: parent
                cursorShape: "PointingHandCursor"
                onClicked: {
                    root.enabled = false
                }
            }
        }

        Column {
            id: mainContent
            width: loginDialog.inputWidth
            spacing: loginDialog.inputSpacing
            anchors {
                horizontalCenter: parent.horizontalCenter
                verticalCenter: parent.verticalCenter
            }

            Rectangle {
                width: loginDialog.inputWidth
                height: loginDialog.inputHeight
                radius: height / 2
                color: "#ebebeb"

                TextInput {
                    id: username
                    anchors.fill: parent
                    anchors.leftMargin: loginDialog.inputHeight / 2

                    helperText: "username or email"

                    KeyNavigation.tab: password
                    KeyNavigation.backtab: password
                }
            }

            Rectangle {
                width: loginDialog.inputWidth
                height: loginDialog.inputHeight
                radius: height / 2
                color: "#ebebeb"

                TextInput {
                    id: password
                    anchors.fill: parent
                    anchors.leftMargin: loginDialog.inputHeight / 2

                    helperText: "password"
                    echoMode: TextInput.Password

                    KeyNavigation.tab: username
                    KeyNavigation.backtab: username
                    onFocusChanged: {
                        if (password.focus) {
                            password.selectAll()
                        }
                    }
                }
            }

            Text {
                id: messageText
                width: loginDialog.inputWidth
                height: loginDialog.inputHeight / 2
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter

                text: loginDialog.statusText
            }

            Rectangle {
                width: loginDialog.inputWidth
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
                    cursorShape: "PointingHandCursor"
                    onClicked: {
                        loginDialog.login(username.text, password.text)
                    }
                }
            }
        }

        Text {
            id: closeText
            visible: isCircular()
            anchors {
                horizontalCenter: parent.horizontalCenter
                top: mainContent.bottom
                topMargin: loginDialog.inputSpacing
            }

            text: "Close"
            font.pixelSize: hifi.fonts.pixelSize * 0.8
            font.weight: Font.Bold
            color: "#175d74"

            MouseArea {
                anchors.fill: parent
                cursorShape: "PointingHandCursor"
                onClicked: {
                    root.enabled = false
                }
            }
        }
    }

    onEnabledChanged: {
        if (enabled) {
            username.forceActiveFocus();
        }
    }

    onVisibleChanged: {
        if (!visible) {
            username.text = ""
            password.text = ""
            loginDialog.statusText = ""
        }
    }

    Keys.onPressed: {
        switch (event.key) {
            case Qt.Key_Escape:
            case Qt.Key_Back:
                enabled = false;
                event.accepted = true
                break
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
                break;
        }
    }
}
