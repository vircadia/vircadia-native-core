// login

import QtQuick 2.3
import QtQuick 2.1
import "HFControls"

Item {
    id: root
    anchors.fill: parent
    Text {
        id: title
        width: 325
        height: 26
        font.family: "Graphik"
        font.pixelSize: 28
        color: "#FFFFFF"
        text: "Please Log in"
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        anchors {
            top: root.top
            topMargin: 29
            horizontalCenter: root.horizontalCenter
        }
    }

    Text {
        id: instruction
        width: 425
        height: 22
        font.family: "Graphik"
        font.pixelSize: 14
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        color: "#C4C4C4"
        text: "Be sure you've uploaded your Avatar before signing in."
        anchors {
            left: root.left
            right: root.right
            top: title.bottom
            topMargin: 18
        }
    }

    HFTextField {
        id: organization
        width: 353
        height: 40
        font.family: "Graphik"
        font.pixelSize: 18
        placeholderText: "Organization name"
        color: "#808080"
        seperatorColor: Qt.rgba(1, 1, 1, 0.3)
        anchors {
            top: instruction.bottom
            horizontalCenter: instruction.horizontalCenter
            topMargin: 24
        }
    }

    HFTextField {
        id: username
        width: 353
        height: 40
        font.family: "Graphik"
        font.pixelSize: 18
        placeholderText: "Username"
        color: "#808080"
        seperatorColor: Qt.rgba(1, 1, 1, 0.3)
        anchors {
            top: organization.bottom
            horizontalCenter: instruction.horizontalCenter
            topMargin: 28
        }
    }

    HFTextField {
        id: password
        width: 353
        height: 40
        font.family: "Graphik"
        font.pixelSize: 18
        placeholderText: "Password"
        color: "#808080"
        seperatorColor: Qt.rgba(1, 1, 1, 0.3)
        echoMode: TextInput.Password
        anchors {
            top: username.bottom
            horizontalCenter: instruction.horizontalCenter
            topMargin: 28
        }
    }

    HFButton {
        id: button
        width: 122
        height: 36

        font.family: "Graphik"
        font.pixelSize: 18
        text: "NEXT"

        anchors {
            top: password.bottom
            horizontalCenter: instruction.horizontalCenter
            topMargin: 48
        }
    }
}
