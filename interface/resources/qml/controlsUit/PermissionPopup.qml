import QtQuick 2.5
import QtWebEngine 1.5
import controlsUit 1.0 as HifiControls
import stylesUit 1.0 as HifiStyles

Rectangle {
    id: root
    width: 600
    height: 200
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

    Row {
        id: webAccessHeaderContainer
        
        height: 60
        width: parent.width
        HifiStyles.RalewayBold {
            id: webAccessHeaderText
            text: "REQUEST FOR DEVICE ACCESS"
            anchors.centerIn: parent
            color: hifi.colors.black
            size: 17
        }
    }

    Row {
        id: webAccessInfoContainer
        height: 60
        width: parent.width
        anchors.top: webAccessHeaderContainer.bottom
        HifiStyles.RalewayLight {
            id: webAccessInfoText
            wrapMode: Text.WordWrap
            width: root.width
            horizontalAlignment: Text.AlignHCenter
            text: "This website is attempting to " + root.permissionLanguage[root.currentRequestedPermission] + "."
            size: 15
            color: hifi.colors.black
        }
    }
    
    Row {
        id: permissionsButtonsRow
        height: 100
        width: parent.width
        anchors.top: webAccessInfoContainer.bottom
        Rectangle {
            id: permissionsButtonsContainer
            height: 50
            width: parent.width
            anchors.fill: parent

            property int space: 5

            HifiControls.Button {
                id: leftButton

                anchors.right: parent.horizontalCenter
                anchors.horizontalCenterOffset: -parent.space

                text: "Don't Allow"
                color: hifi.buttons.red
                enabled: true
                height: 25
                onClicked: {
                    root.permissionButtonPressed(0);
                }
            }
            HifiControls.Button {
                id: rightButton

                anchors.left: parent.horizontalCenter
                anchors.horizontalCenterOffset: parent.space

                text: "Yes allow access"
                color: hifi.buttons.blue
                enabled: true
                width: 155
                height: 25
                onClicked: {
                    root.permissionButtonPressed(1);
                }
            }
        }
    }
}