import QtQuick 2.5
import QtWebEngine 1.5
import QtQuick.Layouts 1.3
import controlsUit 1.0 as HifiControls
import stylesUit 1.0 as HifiStyles

Rectangle {
    id: root
    width: 750
    height: 210
    color: hifi.colors.white

    anchors.centerIn: parent
    readonly property var permissionLanguage: ({
        [WebEngineView.MediaAudioCapture]: "access an audio input device",
        [WebEngineView.MediaVideoCapture]: "access a video device, like your webcam",
        [WebEngineView.MediaAudioVideoCapture]: "access an audio input device and video device",
        [WebEngineView.Geolocation]: "access your location",
        [WebEngineView.DesktopVideoCapture]: "capture video from your desktop",
        [WebEngineView.DesktopAudioVideoCapture]: "capture audio and video from your desktop",
        "none": "Undefined permission being requested"
    })
    property string currentRequestedPermission
    signal permissionButtonPressed(int buttonNumber)

    ColumnLayout {
        anchors.fill: parent
        Rectangle {
            id: webAccessHeaderContainer
            height: 75
            anchors.top: parent.top
            Layout.preferredWidth: parent.width

            HifiStyles.RalewayBold {
                id: webAccessHeaderText
                text: "REQUEST FOR DEVICE ACCESS"

                anchors.horizontalCenter: parent.horizontalCenter
                anchors.bottom: parent.bottom
                wrapMode: Text.WordWrap
                color: hifi.colors.black
                size: 30
            }
        }

        Rectangle {
            id: webAccessInfoContainer
            height: 35
            Layout.preferredWidth: parent.width

            HifiStyles.RalewayLight {
                id: webAccessInfoText
                text: "This website is attempting to " + root.permissionLanguage[root.currentRequestedPermission] + "."

                anchors.centerIn: parent
                wrapMode: Text.WordWrap
                size: 20
                color: hifi.colors.black
            }
        }
        
        Rectangle {
            id: permissionsButtonsRow
            height: 100
            Layout.preferredWidth: parent.width
            Layout.topMargin: 35
            property int space: 8
            readonly property int _LEFT_BUTTON: 0
            readonly property int _RIGHT_BUTTON: 1

            HifiControls.Button {
                id: leftButton
                text: "Don't Allow"

                anchors.right: parent.horizontalCenter
                anchors.rightMargin: parent.space
                color: hifi.buttons.red
                enabled: true
                height: 38
                onClicked: {
                    root.permissionButtonPressed(parent._LEFT_BUTTON);
                }
            }
            HifiControls.Button {
                id: rightButton
                text: "Yes allow access"

                anchors.left: parent.horizontalCenter
                anchors.leftMargin: parent.space
                color: hifi.buttons.blue
                enabled: true
                width: 155
                height: 38
                onClicked: {
                    root.permissionButtonPressed(parent._RIGHT_BUTTON);
                }
            }
        }
    }

}