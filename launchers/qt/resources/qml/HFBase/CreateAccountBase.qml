import QtQuick 2.3
import QtQuick 2.1
import "../HFControls"


Item {
    id: root
    anchors.centerIn: parent
    property string titleText: "Sign-in and pick a password"
    property string usernamePlaceholder: "User name"
    property string passwordPlaceholder: "Set a password"
    property int marginLeft: root.width * 0.15

    Image {
        anchors.centerIn: parent
        width: parent.width
        height: parent.height
        mirror: true
        source: PathUtils.resourcePath("images/hifi_window@2x.png");
        transformOrigin: Item.Center
        rotation: 180
    }

    Text {
        id: title
        width: 481
        height: 27
        font.family: "Graphik"
        font.bold: true
        font.pixelSize: 29
        lineHeight: 35
        lineHeightMode: Text.FixedHeight
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
        font.pixelSize: 14
        color: "#C4C4C4"
        text: "Use the email address that you regisetered with. " + LauncherState.lastSignupError
        anchors {
            left: root.left
            leftMargin: root.marginLeft
            top: title.bottom
            topMargin: 18
        }
    }

    HFTextField {
        id: email
        width: 430
        height: 50
        font.family: "Graphik"
        font.pixelSize: 18
        placeholderText: "Email Address"
        color: "#7e8c81"
        seperatorColor: Qt.rgba(1, 1, 1, 0.3)
        anchors {
            top: instruction.bottom
            left: root.left
            leftMargin: root.marginLeft
            topMargin: 18
        }
    }

    HFTextField {
        id: username
        width: 430
        height: 50
        font.family: "Graphik"
        font.pixelSize: 18
        placeholderText: root.usernamePlaceholder
        color: "#7e8c81"
        seperatorColor: Qt.rgba(1, 1, 1, 0.3)
        anchors {
            top: email.bottom
            left: root.left
            leftMargin: root.marginLeft
            topMargin: 18
        }
    }

    HFTextField {
        id: passwordField
        width: 430
        height: 50
        font.family: "Graphik"
        font.pixelSize: 18
        placeholderText: root.passwordPlaceholder
        color: "#7e8c81"
        seperatorColor: Qt.rgba(1, 1, 1, 0.3)
        togglePasswordField: true
        echoMode: TextInput.Password
        anchors {
            top: username.bottom
            left: root.left
            leftMargin: root.marginLeft
            topMargin: 18
        }
    }


    Text {
        id: displayNameText
        text: "You can change this at anytime from you Profile"
        font.family: "Graphik"
        font.pixelSize: 10
        color: "#C4C4C4"
        anchors {
            top: passwordField.bottom
            left: root.left
            leftMargin: root.marginLeft
            topMargin: 22
        }
    }

     HFTextField {
        id: displayName
        width: 430
        height: 50
        font.family: "Graphik"
        font.pixelSize: 18
        placeholderText: "Display Name"
        color: "#7e8c81"
        seperatorColor: Qt.rgba(1, 1, 1, 0.3)
        anchors {
            top: displayNameText.bottom
            left: root.left
            leftMargin: root.marginLeft
            topMargin: 4
        }
    }

    HFButton {
        id: button
        width: 134
        height: 50

        font.family: "Graphik"
        font.pixelSize: 14
        text: "NEXT"

        anchors {
            top: displayName.bottom
            left: root.left
            leftMargin: root.marginLeft
            topMargin: 21
        }

        onClicked: LauncherState.signup(email.text, username.text, passwordField.text, displayName.text)
    }


    Text {
        width: 214
        height: 12

        text: "Already have an account?"
        font.family: "Graphik"
        font.pixelSize: 14
        color: "#009EE0"

        anchors {
            top: button.bottom
            topMargin: 19
            left: button.left
        }

        MouseArea {
            anchors.fill: parent

            onClicked: {
                console.log("clicked");
                LauncherState.gotoLogin();
            }
        }
    }

    Text {
        width: 100
        height: 17

        text: "High Fidelity"
        font.bold: true
        font.family: "Graphik"
        font.pixelSize: 24
        font.letterSpacing: -1
        color: "#FFFFFF"
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter

        anchors {
            bottom: root.bottom
            bottomMargin: 46
            right: displayName.right
            rightMargin: 30
        }
    }
}
