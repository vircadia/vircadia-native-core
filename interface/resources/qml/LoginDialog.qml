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

DialogContainer {
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

            MouseArea {
                width: parent.width
                height: parent.height
                anchors {
                    horizontalCenter: parent.horizontalCenter
                    verticalCenter: parent.verticalCenter
                }
                drag {
                    target: root
                    minimumX: 0
                    minimumY: 0
                    maximumX: root.parent ? root.maximumX : 0
                    maximumY: root.parent ? root.maximumY : 0
                }
            }
        }

        Image {
            id: closeIcon
            source: "../images/login-close.svg"
            width: 20
            height: 20
            anchors {
                top: backgroundRectangle.top
                right: backgroundRectangle.right
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

            Row {
                anchors.horizontalCenter: parent.horizontalCenter

                Text {
                    text: "Password?"
                    font.pixelSize: hifi.fonts.pixelSize * 0.8
                    font.underline: true
                    color: "#e0e0e0"
                    width: loginDialog.inputHeight * 4
                    horizontalAlignment: Text.AlignRight
                    anchors.verticalCenter: parent.verticalCenter

                    MouseArea {
                        anchors.fill: parent
                        cursorShape: "PointingHandCursor"
                        onClicked: {
                            loginDialog.openUrl(loginDialog.rootUrl + "/users/password/new")
                        }
                    }
                }

                Item {
                    width: loginDialog.inputHeight + loginDialog.inputSpacing * 2
                    height: loginDialog.inputHeight

                    Image {
                        id: hifiIcon
                        source: "../images/hifi-logo-blackish.svg"
                        width: loginDialog.inputHeight
                        height: width
                        anchors {
                            horizontalCenter: parent.horizontalCenter
                            verticalCenter: parent.verticalCenter
                        }
                    }
                }

                Text {
                    text: "Register"
                    font.pixelSize: hifi.fonts.pixelSize * 0.8
                    font.underline: true
                    color: "#e0e0e0"
                    width: loginDialog.inputHeight * 4
                    horizontalAlignment: Text.AlignLeft
                    anchors.verticalCenter: parent.verticalCenter

                    MouseArea {
                        anchors.fill: parent
                        cursorShape: "PointingHandCursor"
                        onClicked: {
                            loginDialog.openUrl(loginDialog.rootUrl + "/signup")
                        }
                    }
                }
            }
        }
    }

    onOpacityChanged: {
        // Set focus once animation is completed so that focus is set at start-up when not logged in
        if (opacity == 1.0) {
            username.forceActiveFocus()
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
                enabled = false
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
                break
        }
    }
}
