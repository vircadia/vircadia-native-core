import QtQuick 2.3
import QtQuick 2.1
import "../HFControls"


Item {
    id: root
    anchors.centerIn: parent
    property string titleText: "Create your account"
    property string usernamePlaceholder: "User name"
    property string passwordPlaceholder: "Set a password"
    property int marginLeft: root.width * 0.15

    Image {
        anchors.centerIn: parent
        width: parent.width
        height: parent.height
        mirror: true
        source: "qrc:/images/hifi_window@2x.png"
        transformOrigin: Item.Center
        rotation: 180
    }

    Text {
        id: title
        width: 325
        height: 26
        font.family: "Graphik"
        font.bold: true
        font.pixelSize: 24
        color: "#FFFFFF"
        text: root.titleText
        anchors {
            top: root.top
            topMargin: 29
            left: root.left
            leftMargin: root.marginLeft
        }
    }

    Text {
        id: instruction
        width: 425
        height: 22
        font.family: "Graphik"
        font.pixelSize: 10
        color: "#C4C4C4"
        text: "Use the email address that you regisetered with."
        anchors {
            left: root.left
            leftMargin: root.marginLeft
            top: title.bottom
            topMargin: 9
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
            left: root.left
            leftMargin: root.marginLeft
            topMargin: 4
        }
    }

    HFTextField {
        id: username
        width: 353
        height: 40
        font.family: "Graphik"
        font.pixelSize: 18
        placeholderText: root.usernamePlaceholder
        color: "#808080"
        seperatorColor: Qt.rgba(1, 1, 1, 0.3)
        anchors {
            top: organization.bottom
            left: root.left
            leftMargin: root.marginLeft
            topMargin: 10
        }
    }

    HFTextField {
        id: passwordField
        width: 353
        height: 40
        font.family: "Graphik"
        font.pixelSize: 18
        placeholderText: root.passwordPlaceholder
        color: "#808080"
        seperatorColor: Qt.rgba(1, 1, 1, 0.3)
        echoMode: TextInput.Password
        anchors {
            top: username.bottom
            left: root.left
            leftMargin: root.marginLeft
            topMargin: 10
        }
    }


    Text {
        id: displayNameText
        text: "This is the display name other people see in world. It can be changed at \nanytime, from your profile"
        font.family: "Graphik"
        font.pixelSize: 10
        color: "#FFFFFF"
        anchors {
            top: passwordField.bottom
            left: root.left
            leftMargin: root.marginLeft
            topMargin: 15
        }
    }

     HFTextField {
        id: displayName
        width: 353
        height: 40
        font.family: "Graphik"
        font.pixelSize: 18
        placeholderText: "Password"
        color: "#C4C4C4"
        seperatorColor: Qt.rgba(1, 1, 1, 0.3)
        anchors {
            top: displayNameText.bottom
            left: root.left
            leftMargin: root.marginLeft
            topMargin: 8
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
            top: displayName.bottom
            left: root.left
            leftMargin: root.marginLeft
            topMargin: 15
        }

        onClicked: LauncherState.login(username.text, passwordField.text)
    }
}
