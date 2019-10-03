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
    HFTextHeader {
        id: title
        width: 325
        height: 26
        font.bold: true
        text: "Please Log in"
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        anchors {
            top: root.top
            topMargin: 40
            horizontalCenter: root.horizontalCenter
        }
    }

    HFTextRegular {
        id: instruction
        width: 425
        height: 22

        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter

        visible: LauncherState.lastLoginErrorMessage.length == 0
        text: "Use the account credentials you created at sign-up"
        anchors {
            left: root.left
            right: root.right
            top: title.bottom
            topMargin: 18
        }
    }

    HFTextRegular {
        id: error
        width: 425
        height: 22

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

        placeholderText: "Username"

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
        placeholderText: "Password"
        togglePasswordField: true
        echoMode: TextInput.Password
        seperatorColor: Qt.rgba(1, 1, 1, 0.3)
        anchors {
            top: username.bottom
            horizontalCenter: instruction.horizontalCenter
            topMargin: 25
        }
    }


    HFTextRegular {
        id: displayText

        text: "You can change this at anytime from your profile."

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
        placeholderText: "Display name"
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

    HFTextLogo {
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
