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

    Item {
        width: 353
        height: root.height


        anchors {
            top: root.top
            horizontalCenter: root.horizontalCenter
        }

        HFTextHeader {
            id: title

            font.bold: true

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
                left: parent.left
                right: parent.right;

                top: title.bottom
                topMargin: 18
            }
        }

        HFTextError {
            id: error

            visible: LauncherState.lastLoginErrorMessage.length > 0
            text: LauncherState.lastLoginErrorMessage
            anchors {
                top: title.bottom
                topMargin: 18

                left: parent.left
                right: parent.right;
            }
        }

        HFTextField {
            id: username

            placeholderText: "Username"

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

            text: "You can change this at anytime from your profile."

            anchors {
                top: password.bottom
                topMargin: 50

                left: parent.left
                right: parent.right;
            }
        }

        HFTextField {
            id: displayName

            placeholderText: "Display name"
            seperatorColor: Qt.rgba(1, 1, 1, 0.3)
            anchors {
                top: displayText.bottom
                topMargin: 4

                left: parent.left
                right: parent.right;
            }
        }

        HFButton {
            id: button
            width: 110

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

            text: "Create New Account"
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
