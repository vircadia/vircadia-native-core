import QtQuick 2.5
import QtWebEngine 1.5
import controlsUit 1.0 as HifiControls
import stylesUit 1.0 as HifiStyles
import "../windows" as Windows
import "../."

Item {
    id: root
    width:  600
    height: 200
    z:100
    anchors.horizontalCenter: parent.horizontalCenter
    anchors.verticalCenter: parent.verticalCenter
    readonly property var permissionLanguage: ({
        [WebEngineView.MediaAudioCapture]: "access an audio input device",
        [WebEngineView.MediaVideoCapture]: "access a video device, like your webcam",
        [WebEngineView.MediaAudioVideoCapture]: "access an audio input device and video device",
        [WebEngineView.Geolocation]: "access your location",
        [WebEngineView.DesktopVideoCapture]: "capture video from your desktop",
        [WebEngineView.DesktopAudioVideoCapture]: "capture audio and video from your desktop"
    })
    property string currentRequestedPermission
    signal permissionButtonPressed(real buttonNumber)
    
    Rectangle {
        id: mainContainer
        width: root.width
        height: root.height
        color: hifi.colors.white

        Row {
            id: webAccessHeaderContainer
            height: root.height * 0.30
            HifiStyles.RalewayBold {
                id: webAccessHeaderText
                text: "REQUEST FOR DEVICE ACCESS"
                width: mainContainer.width
                horizontalAlignment: Text.AlignHCenter
                anchors.bottom: parent.bottom
                font.bold: true
                color: hifi.colors.black
                size: 17
            }
        }

        Row {
            id: webAccessInfoContainer
            anchors.top: webAccessHeaderContainer.bottom
            anchors.topMargin: 10
            HifiStyles.RalewayLight {
                width: mainContainer.width
                id: webAccessInfoText
                horizontalAlignment: Text.AlignHCenter
                text: "This website is attempting to " + root.permissionLanguage[root.currentRequestedPermission] + "."
                size: 15
                color: hifi.colors.black
            }
        }
        
        Rectangle {
            id: permissionsButtonRow
            color: "#AAAAAA"
            anchors.topMargin: 35
            height: 50
            width: leftButton.width + rightButton.width + (this.space * 3)
            anchors.top: webAccessInfoContainer.bottom
            anchors.horizontalCenter: webAccessInfoContainer.horizontalCenter
            anchors.verticalCenter: webAccessInfoContainer.verticalCenter
            property real space: 5
            HifiControls.Button {
                anchors.left: permissionsButtonRow.left
                id: leftButton
                anchors.leftMargin: permissionsButtonRow.space

                text: "Yes allow access"
                color: hifi.buttons.blue
                enabled: true
                width: 155
                onClicked: {
                    root.permissionButtonPressed(0)
                }
            }
            HifiControls.Button {  
                id: rightButton
                anchors.left: leftButton.right
                anchors.leftMargin: permissionsButtonRow.space
                text: "Don't Allow"
                color: hifi.buttons.red
                enabled: true
                onClicked: {
                    root.permissionButtonPressed(1)
                }
            }
        }
    }
}