import Hifi 1.0
import QtQuick 2.3
import QtQuick.Controls.Styles 1.3
import "controls"
import "styles"

Dialog {
    HifiConstants { id: hifi }
    title: "Login"
    objectName: "LoginDialog"
    height: 512
    width: 384

    onVisibleChanged: {
        if (!visible) {
            reset()
        }
    }

    onEnabledChanged: {
        if (enabled) {
            username.forceActiveFocus();
        }
    }

    function reset() {
        username.text = ""
        password.text = ""
        loginDialog.statusText = ""
    }

    LoginDialog {
        id: loginDialog
        anchors.fill: parent
        anchors.margins: parent.margins
        anchors.topMargin: parent.topMargin
        Column {
            anchors.topMargin: 8
            anchors.right: parent.right
            anchors.rightMargin: 0
            anchors.left: parent.left
            anchors.top: parent.top
            spacing: 8

            Image {
                height: 64
                anchors.horizontalCenter: parent.horizontalCenter
                width: 64
                source: "../images/hifi-logo.svg"
            }

            Border {
                width: 304
                height: 64
                anchors.horizontalCenter: parent.horizontalCenter
                TextInput {
                    id: username
                    anchors.fill: parent
                    helperText: "Username or Email"
                    anchors.margins: 8
                    KeyNavigation.tab: password
                    KeyNavigation.backtab: password
                }
            }

            Border {
                width: 304
                height: 64
                anchors.horizontalCenter: parent.horizontalCenter
                TextInput {
                    id: password
                    anchors.fill: parent
                    echoMode: TextInput.Password
                    helperText: "Password"
                    anchors.margins: 8
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
                anchors.horizontalCenter: parent.horizontalCenter
                textFormat: Text.StyledText
                width: parent.width
                height: 96
                wrapMode: Text.WordWrap
                verticalAlignment: Text.AlignVCenter
                horizontalAlignment: Text.AlignHCenter
                text: loginDialog.statusText
            }
        }

        Column {
            anchors.bottomMargin: 5
            anchors.right: parent.right
            anchors.rightMargin: 0
            anchors.left: parent.left
            anchors.bottom: parent.bottom

            Rectangle {
                width: 192
                height: 64
                anchors.horizontalCenter: parent.horizontalCenter
                color: hifi.colors.hifiBlue
                border.width: 0
                radius: 10

                MouseArea {
                    anchors.bottom: parent.bottom
                    anchors.bottomMargin: 0
                    anchors.top: parent.top
                    anchors.right: parent.right
                    anchors.left: parent.left
                    onClicked: {
                        loginDialog.login(username.text, password.text)
                    }
                }

                Row {
                    anchors.centerIn: parent
                    anchors.verticalCenter: parent.verticalCenter
                    spacing: 8
                    Image {
                        id: loginIcon
                        height: 32
                        width: 32
                        source: "../images/login.svg"
                    }
                    Text {
                        text: "Login"
                        color: "white"
                        width: 64
                        height: parent.height
                    }
                }

            }

            Text {
                width: parent.width
                height: 24
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                text:"Create Account"
                font.pointSize: 12
                font.bold: true
                color: hifi.colors.hifiBlue

                MouseArea {
                    anchors.fill: parent
                    onClicked: {
                        loginDialog.openUrl(loginDialog.rootUrl + "/signup")
                    }
                }
            }

            Text {
                width: parent.width
                height: 24
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                font.pointSize: 12
                text: "Recover Password"
                color: hifi.colors.hifiBlue

                MouseArea {
                    anchors.fill: parent
                    onClicked: {
                        loginDialog.openUrl(loginDialog.rootUrl + "/users/password/new")
                    }
                }
            }
        }
    }
    Keys.onPressed: {
        switch(event.key) {
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
