import QtQuick 2.6

import "../../controlsUit" 1.0 as HifiControls
import "../../stylesUit" 1.0

Rectangle {
    id: root 

    width: parent.width
    height: 74
    color: "#252525"

    property alias title: title.text
    property alias docsEnabled: docs.visible
    property bool backButtonVisible: true // If false, is not visible and does not take up space
    property bool backButtonEnabled: true // If false, is not visible but does not affect space
    property bool canRename: false
    property int colorScheme

    property color textColor: "white"
    property color hoverTextColor: "gray"
    property color pressedTextColor: "#6A6A6A"

    signal backButtonClicked
    signal docsButtonClicked

    RalewaySemiBold {
        id: back
        visible: backButtonEnabled && backButtonVisible
        size: 28
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.leftMargin: 16
        anchors.verticalCenter: back.verticalCenter
        text: "◀"
        color: textColor
        MouseArea {
            anchors.fill: parent
            onClicked: root.backButtonClicked()
            hoverEnabled: true
            onEntered: { state = "hovering" }
            onExited: { state = "" }
            states: [
                State {
                    name: "hovering"
                    PropertyChanges { target: back; color: hoverTextColor }
                }
            ]
        }
    }
    Item {
        id: titleArea
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.left: root.backButtonVisible ? back.right : parent.left
        anchors.leftMargin: root.backButtonVisible ? 11 : 21
        anchors.verticalCenter: title.verticalCenter
        anchors.right: docs.left
        states: [
            State {
                name: "renaming"
                PropertyChanges { target: title; visible: false }
                PropertyChanges { target: titleInputArea; visible: true }
            }
        ]

        RalewaySemiBold {
            id: title
            size: 28
            anchors.fill: parent
            text: qsTr("Avatar Packager")
            color: "white"

            MouseArea {
                anchors.fill: parent
                onClicked: {
                    if (!root.canRename || AvatarPackagerCore.currentAvatarProject === null) {
                        return;
                    }

                    titleArea.state = "renaming";
                    titleInput.text = AvatarPackagerCore.currentAvatarProject.name;
                    titleInput.selectAll();
                    titleInput.forceActiveFocus(Qt.MouseFocusReason);
                }
            }
        }
        Item {
            id: titleInputArea
            visible: false
            anchors.fill: parent

            HifiControls.TextField {
                id: titleInput
                anchors.fill: parent
                text: ""
                colorScheme: root.colorScheme
                font.family: "Fira Sans"
                font.pixelSize: 28
                z: 200
                onFocusChanged: {
                    if (titleArea.state === "renaming" && !titleArea.focus) {
                        //titleArea.state = "";
                        accepted();
                    }
                }
                Keys.onPressed: {
                    if (event.key === Qt.Key_Escape) {
                        titleArea.state = "";
                    }
                }
                onAccepted:  {
                    if (acceptableInput) {
                        //AvatarPackagerCore.renameProject(text);
                        console.warn(text);
                        AvatarPackagerCore.currentAvatarProject.name = text;
                        console.warn(AvatarPackagerCore.currentAvatarProject.name);

                    }
                    titleArea.state = "";
                }
            }
        }
    }

    RalewaySemiBold {
        id: docs
        visible: false
        size: 28
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.right: parent.right
        anchors.rightMargin: 16
        anchors.verticalCenter: docs.verticalCenter
        text: qsTr("Docs")
        color: "white"
        MouseArea {
            anchors.fill: parent
            onClicked: {
                docsButtonClicked();
            }
        }
    }
}
