import QtQuick 2.5
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
    signal permissionButtonPressed(real buttonNumber)

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
                text: "WEB CAMERA ACCESS REQUEST"
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
                text: "This domain is requesting access to your web camera and microphone"
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