import QtQuick 2.3
import QtQuick 2.1
import "../HFControls"

Item {
    id: root
    anchors.fill: parent

    Image {
        anchors.centerIn: parent
        width: parent.width
        height: parent.height
        mirror: false
        source: PathUtils.resourcePath("images/hifi_window@2x.png");
        transformOrigin: Item.Center
        rotation: 0
    }
    Text {
        id: title
        width: 325
        height: 26
        font.family: "Graphik"
        font.pixelSize: 28
        font.bold: true
        color: "#FFFFFF"
        text: "Please Log in"
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        anchors {
            top: root.top
            topMargin: 40
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
        visible: LauncherState.lastLoginErrorMessage.length == 0
        text: "Use the account credentials you created at sign-up"
        anchors {
            left: root.left
            right: root.right
            top: title.bottom
            topMargin: 18
        }
    }

    Text {
        id: error
        width: 425
        height: 22
        font.family: "Graphik"
        font.pixelSize: 14
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        color: "#FF9999"
        visible: LauncherState.lastLoginErrorMessage.length > 0
        text: LauncherState.lastLoginErrorMessage
        anchors {
            left: root.left
            right: root.right
            top: title.bottom
            topMargin: 18
        }
    }

    HFTextField {
        id: username
        width: 353
        height: 50
        font.family: "Graphik"
        font.pixelSize: 18
        placeholderText: "Username"
        color: "#7E8C81"
        seperatorColor: Qt.rgba(1, 1, 1, 0.3)
        anchors {
            top: error.bottom
            horizontalCenter: error.horizontalCenter
            topMargin: 24
        }
    }

    HFTextField {
        id: password
        width: 353
        height: 50
        font.family: "Graphik"
        font.pixelSize: 18
        placeholderText: "Password"
        color: "#7E8C81"
        togglePasswordField: true
        echoMode: TextInput.Password
        seperatorColor: Qt.rgba(1, 1, 1, 0.3)
        anchors {
            top: username.bottom
            horizontalCenter: instruction.horizontalCenter
            topMargin: 25
        }
    }


    Text {
        id: displayText

        text: "You can change this at anytime from your profile"
        color: "#C4C4C4"
        font.pixelSize: 14

        anchors {
            top: password.bottom
            topMargin: 50
            left: password.left
        }
    }

    HFTextField {
        id: displayName
        width: 353
        height: 50
        font.family: "Graphik"
        font.pixelSize: 18
        placeholderText: "Display name"
        color: "#7E8C81"
        seperatorColor: Qt.rgba(1, 1, 1, 0.3)
        anchors {
            top: displayText.bottom
            horizontalCenter: instruction.horizontalCenter
            topMargin: 4
        }
    }

    HFButton {
        id: button
        width: 110
        height: 50

        font.family: "Graphik"
        font.pixelSize: 18
        text: "NEXT"

        anchors {
            top: displayName.bottom
            left: displayName.left
            topMargin: 25
        }

        onClicked: LauncherState.login(username.text, password.text, displayName.text)
    }

    Text {
        width: 214
        height: 12

        text: "Create New Account"
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
                LauncherState.gotoSignup();
            }
        }
    }

    Text {
        width: 100
        height: 17

        text: "High Fidelity"
        font.bold: true
        font.family: "Graphik"
        font.pixelSize: 18
        font.letterSpacing: -1
        color: "#FFFFFF"
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter

        anchors {
            bottom: root.bottom
            bottomMargin: 58
            right: root.right
            rightMargin: 136
        }
    }

    Component.onCompleted: {
        root.parent.setStateInfoState("left");
        root.parent.setBuildInfoState("right");
    }
}
