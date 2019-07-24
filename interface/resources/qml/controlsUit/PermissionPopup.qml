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
    anchors.horizontalCenter: parent.horizontalCenter
    anchors.verticalCenter: parent.verticalCenter
    z:100
    property var permissionLanguage: {
        "test": "test"
    }
    property int currentRequestedPermission
    signal permissionButtonPressed(real buttonNumber)
    
    Component.onCompleted: {
        console.log("loaded component");
        // console.log("\n\n TESTING!! \n\n")
        console.log("WebEngineView.MediaAudioCapture", WebEngineView.MediaAudioCapture)
        // root.permissionLanguage["test"] = "test"
        root.permissionLanguage[WebEngineView.MediaAudioCapture] = "access an audio input device";
        root.permissionLanguage[WebEngineView.MediaVideoCapture] = "access a video device, like your webcam";
        root.permissionLanguage[WebEngineView.MediaAudioVideoCapture] = "access an audio input device and video device";
        root.permissionLanguage[WebEngineView.Geolocation] = "access your location";
        root.permissionLanguage[WebEngineView.DesktopVideoCapture] = "capture video from your desktop";
        root.permissionLanguage[WebEngineView.DesktopAudioVideoCapture] = "capture audio and video from your desktop";
        console.log(JSON.stringify(root.permissionLanguage))

    }

    // anchors.top: buttons.bottom
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
                // colorScheme: root.colorScheme
                enabled: true
                width: 155
                onClicked: {
                    console.log("\n\n JUST CLICKED BUTTON 0, GOING TO SEND SIGNAL!")
                    root.permissionButtonPressed(0)
                }
            }
            HifiControls.Button {  
                id: rightButton
                anchors.left: leftButton.right
                anchors.leftMargin: permissionsButtonRow.space
                text: "Don't Allow"
                color: hifi.buttons.red
                // colorScheme: root.colorScheme
                enabled: true
                onClicked: {
                    console.log("\n\n JUST CLICKED BUTTON 1, GOING TO SEND SIGNAL!")
                    root.permissionButtonPressed(1)
                }
            }
        }
    }
}