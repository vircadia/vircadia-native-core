import QtQuick 2.3
import QtQuick 2.1

import "../HFControls"
import HQLauncher 1.0


Item {
    id: root
    anchors.centerIn: parent
    property string titleText: "Create Your Username and Password"
    property string usernamePlaceholder: "Username"
    property string passwordPlaceholder: "Set a password (must be at least 6 characters)"
    property int marginLeft: root.width * 0.15

    property bool enabled: LauncherState.applicationState == ApplicationState.WaitingForSignup

    Image {
        anchors.centerIn: parent
        width: parent.width
        height: parent.height
        mirror: true
        source: PathUtils.resourcePath("images/hifi_window@2x.png");
        transformOrigin: Item.Center
        rotation: 180
    }

    HFTextHeader {
        id: title
        width: 481
        wrapMode: Text.WordWrap
        lineHeight: 35
        lineHeightMode: Text.FixedHeight
        text: LauncherState.lastSignupErrorMessage.length == 0 ? root.titleText : "Uh oh"
        anchors {
            top: root.top
            topMargin: 29
            left: root.left
            leftMargin: root.marginLeft
        }
    }

    HFTextError {
        id: error

        width: 425

        wrapMode: Text.Wrap

        visible: LauncherState.lastSignupErrorMessage.length > 0
        text: LauncherState.lastSignupErrorMessage

        textFormat: Text.StyledText

        onLinkActivated: {
            if (link == "login") {
                LauncherState.gotoLogin();
            } else {
                LauncherState.openURLInBrowser(link)
            }
        }

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

        enabled: root.enabled

        placeholderText: "Verify Your Email"
        seperatorColor: Qt.rgba(1, 1, 1, 0.3)
        anchors {
            top: error.visible ? error.bottom : title.bottom
            left: root.left
            leftMargin: root.marginLeft
            topMargin: 18
        }
    }

    HFTextField {
        id: username
        width: 430

        enabled: root.enabled

        placeholderText: root.usernamePlaceholder
        seperatorColor: Qt.rgba(1, 1, 1, 0.3)
        anchors {
            top: email.bottom
            left: root.left
            leftMargin: root.marginLeft
            topMargin: 18
        }
    }

    HFTextField {
        id: password
        width: 430

        enabled: root.enabled

        placeholderText: root.passwordPlaceholder
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


    HFTextRegular {
        id: displayNameText

        text: "This is the display name other people see in High Fidelity. It can be changed at any time from your profile."
        wrapMode: Text.Wrap

        width: 430

        anchors {
            top: password.bottom
            left: root.left
            leftMargin: root.marginLeft
            topMargin: 22
        }
    }

     HFTextField {
        id: displayName
        width: 430

        enabled: root.enabled

        placeholderText: "Display Name"
        seperatorColor: Qt.rgba(1, 1, 1, 0.3)
        anchors {
            top: displayNameText.bottom
            left: root.left
            leftMargin: root.marginLeft
            topMargin: 4
        }

         onAccepted: {
             if (root.enabled && email.text.length > 0 && username.text.length > 0 && password.text.length > 0 && displayName.text.length > 0) {
                 LauncherState.signup(email.text, username.text, password.text, displayName.text);
             }
         }
    }

    HFButton {
        id: button
        width: 134

        enabled: root.enabled && email.text.length > 0 && username.text.length > 0 && password.text.length > 0 && displayName.text.length > 0

        text: "NEXT"

        anchors {
            top: displayName.bottom
            left: root.left
            leftMargin: root.marginLeft
            topMargin: 21
        }

        onClicked: LauncherState.signup(email.text, username.text, password.text, displayName.text)
    }


    Text {
        text: "Already have an account?"
        font.family: "Graphik"
        font.pixelSize: 14
        color: "#009EE0"

        anchors {
            top: button.bottom
            topMargin: 16
            left: button.left
        }

        MouseArea {
            anchors.fill: parent

            cursorShape: Qt.PointingHandCursor

            onClicked: {
                LauncherState.gotoLogin();
            }
        }
    }

    HFTextLogo {
        anchors {
            bottom: root.bottom
            bottomMargin: 46
            right: displayName.right
        }
    }

    Component.onCompleted: {
        root.parent.setBuildInfoState("left");
    }
}
