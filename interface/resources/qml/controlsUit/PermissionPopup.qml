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
            Layout.preferredHeight: 75
            Layout.preferredWidth: parent.width

            HifiStyles.RalewayBold {
                text: "REQUEST FOR DEVICE ACCESS"

                anchors.horizontalCenter: parent.horizontalCenter
                anchors.bottom: parent.bottom
                wrapMode: Text.WordWrap
                color: hifi.colors.black
                size: 30
            }
        }

        Rectangle {
            Layout.preferredHeight: 35
            Layout.preferredWidth: parent.width

            HifiStyles.RalewayLight {
                text: "This website is attempting to " + root.permissionLanguage[root.currentRequestedPermission] + "."

                anchors.centerIn: parent
                wrapMode: Text.Wrap
                size: 20
                color: hifi.colors.black
            }
        }
        
        Rectangle {
            Layout.preferredHeight: 100
            Layout.preferredWidth: parent.width
            Layout.topMargin: 35
            property int space: 8

            HifiControls.Button {
                text: "Don't Allow"

                anchors.right: parent.horizontalCenter
                anchors.rightMargin: parent.space
                width: 125
                color: hifi.buttons.red
                height: 38
                onClicked: {
                    root.permissionButtonPressed(0);
                }
            }
            HifiControls.Button {
                text: "Yes allow access"

                anchors.left: parent.horizontalCenter
                anchors.leftMargin: parent.space
                color: hifi.buttons.blue
                width: 155
                height: 38
                onClicked: {
                    root.permissionButtonPressed(1);
                }
            }
        }
    }

}
