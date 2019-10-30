import QtQuick 2.3
import QtQuick 2.1

import "../HFControls"
import HQLauncher 1.0

Item {
    id: root
    anchors.fill: parent

    property bool enabled: LauncherState.applicationState == ApplicationState.WaitingForLogin

    Image {
        anchors.centerIn: parent
        width: parent.width
        height: parent.height
        mirror: false
        source: PathUtils.resourcePath("images/hifi_window@2x.png");
        transformOrigin: Item.Center
        rotation: 0
    }

    Item {
        width: 430
        height: root.height


        anchors {
            top: root.top
            horizontalCenter: root.horizontalCenter
        }

        HFTextHeader {
            id: title
            lineHeight: 35
            lineHeightMode: Text.FixedHeight
            text: "Please Log in"

            anchors {
                top: parent.top
                topMargin: 40

                horizontalCenter: parent.horizontalCenter
            }
        }

        HFTextRegular {
            id: instruction

            visible: LauncherState.lastLoginErrorMessage.length == 0
            text: "Use the account credentials you created at sign-up"
            anchors {
                top: title.bottom
                topMargin: 18

                horizontalCenter: parent.horizontalCenter
            }
        }

        HFTextError {
            id: error

            visible: LauncherState.lastLoginErrorMessage.length > 0
            text: LauncherState.lastLoginErrorMessage
            anchors {
                top: title.bottom
                topMargin: 18

                horizontalCenter: parent.horizontalCenter
            }
        }

        HFTextField {
            id: username

            enabled: root.enabled
            width: 430

            text: LauncherState.lastUsedUsername
            placeholderText: "Username or Email address"

            seperatorColor: Qt.rgba(1, 1, 1, 0.3)
            anchors {
                top: error.bottom
                topMargin: 24

                left: parent.left
                right: parent.right;
            }
        }

        HFTextField {
            id: password
            width: 430
            enabled: root.enabled

            placeholderText: "Password"
            togglePasswordField: true
            echoMode: TextInput.Password
            seperatorColor: Qt.rgba(1, 1, 1, 0.3)
            anchors {
                top: username.bottom
                topMargin: 25

                left: parent.left
                right: parent.right;
            }
        }


        HFTextRegular {
            id: displayText

            text: "This is the display name other people see in High Fidelity. It can be changed at any time from your profile."
            wrapMode: Text.Wrap

            anchors {
                top: password.bottom
                topMargin: 50

                left: parent.left
                right: parent.right;
            }
        }

        HFTextField {
            id: displayName
            width: 430
            enabled: root.enabled

            placeholderText: "Display name"
            seperatorColor: Qt.rgba(1, 1, 1, 0.3)
            anchors {
                top: displayText.bottom
                topMargin: 4

                left: parent.left
                right: parent.right;
            }
            onAccepted: {
                if (root.enabled && username.text.length > 0 && password.text.length > 0 && displayName.text.length > 0) {
                    LauncherState.login(username.text, password.text, displayName.text);
                }
            }
        }

        HFButton {
            id: button
            width: 134

            enabled: root.enabled && username.text.length > 0 && password.text.length > 0 && displayName.text.length > 0

            text: "NEXT"

            anchors {
                top: displayName.bottom
                topMargin: 25

                left: parent.left
            }

            onClicked: LauncherState.login(username.text, password.text, displayName.text)
        }

        Text {
            id: createAccountLink

            text: "Sign up"
            font.family: "Graphik"
            font.pixelSize: 14
            color: "#009EE0"

            anchors {
                top: button.bottom
                topMargin: 16
                left: parent.left
            }

            MouseArea {
                anchors.fill: parent

                cursorShape: Qt.PointingHandCursor

                onClicked: {
                    console.log("clicked");
                    LauncherState.gotoSignup();
                }
            }
        }

        HFTextLogo {
            anchors {
                bottom: createAccountLink.bottom

                right: parent.right
            }
        }

    }
    Component.onCompleted: {
        root.parent.setBuildInfoState("right");
    }
}
