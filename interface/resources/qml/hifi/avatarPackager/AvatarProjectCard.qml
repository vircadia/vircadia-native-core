import QtQuick 2.0

import "../../controlsUit" 1.0 as HifiControls
import "../../stylesUit" 1.0


Item {
    id: projectCard
    height: 80
    width: parent.width

    property alias title: title.text
    property alias path: path.text

    property color textColor: "#E3E3E3"
    property color hoverTextColor: "#121212"
    property color pressedTextColor: "#121212"

    property color backgroundColor: "#121212"
    property color hoverBackgroundColor: "#E3E3E3"
    property color pressedBackgroundColor: "#6A6A6A"

    state: mouseArea.pressed ? "pressed" : (mouseArea.containsMouse ? "hover" : "normal")
    states: [
        State {
            name: "normal"
            PropertyChanges { target: background; color: backgroundColor }
            PropertyChanges { target: title; color: textColor }
            PropertyChanges { target: path; color: textColor }
        },
        State {
            name: "hover"
            PropertyChanges { target: background; color: hoverBackgroundColor }
            PropertyChanges { target: title; color: hoverTextColor }
            PropertyChanges { target: path; color: hoverTextColor }
        },
        State {
            name: "pressed"
            PropertyChanges { target: background; color: pressedBackgroundColor }
            PropertyChanges { target: title; color: pressedTextColor }
            PropertyChanges { target: path; color: pressedTextColor }
        }
    ]

    Rectangle {
        id: background
        width: parent.width
        height: parent.height
        color: "#121212"
        radius: 4

        RalewayBold {
            id: title
            anchors {
                top: parent.top
                topMargin: 13
                left: parent.left
                leftMargin: 16
            }
            text: "<title missing>"
            size: 16
        }

        RalewayRegular {
            id: path
            anchors {
                top: title.bottom
                left: parent.left
                leftMargin: 32
            }
            text: "<path missing>"
            size: 20
        }

        MouseArea {
            id: mouseArea
            anchors.fill: parent
            hoverEnabled: true
            onClicked: {
                AvatarPackagerCore.openAvatarProject(path.text);
                avatarPackager.state = "project";
            }
        }
    }
}
