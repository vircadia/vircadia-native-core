import QtQuick 2.6

import "../../controlsUit" 1.0 as HifiControls
import "../../stylesUit" 1.0
import "../avatarapp" 1.0

ShadowRectangle {
    id: root 

    width: parent.width
    height: 74
    color: "#252525"

    property string title: qsTr("Avatar Packager")
    property alias docsEnabled: docs.visible
    property alias videoEnabled: video.visible
    property bool backButtonVisible: true // If false, is not visible and does not take up space
    property bool backButtonEnabled: true // If false, is not visible but does not affect space
    property bool canRename: false
    property int colorScheme

    property color textColor: "white"
    property color hoverTextColor: "gray"
    property color pressedTextColor: "#6A6A6A"

    signal backButtonClicked
    signal docsButtonClicked
    signal videoButtonClicked

    RalewayButton {
        id: back

        visible: backButtonEnabled && backButtonVisible

        size: 28
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.leftMargin: 16

        text: "◀"

        onClicked: root.backButtonClicked()
    }
    Item {
        id: titleArea

        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.left: root.backButtonVisible ? back.right : parent.left
        anchors.leftMargin: root.backButtonVisible ? 11 : 21
        anchors.right: docs.left
        states: [
            State {
                name: "renaming"
                PropertyChanges { target: title; visible: false }
                PropertyChanges { target: titleInputArea; visible: true }
            }
        ]

        Item {
            id: title
            anchors.fill: parent

            RalewaySemiBold {
                id: titleNotRenameable

                visible: !root.canRename

                size: 28
                anchors.fill: parent
                text: root.title
                color: "white"
            }

            RalewayButton {
                id: titleRenameable

                visible: root.canRename
                enabled: root.canRename

                size: 28
                anchors.fill: parent
                text: root.title

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
                    if (titleArea.state === "renaming" && !focus) {
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
                        AvatarPackagerCore.currentAvatarProject.name = text;
                    }
                    titleArea.state = "";
                }
            }
        }
    }

    // FIXME: Link to a Vircadias version of the video.
    /*
    RalewayButton {
        id: video
        visible: false
        size: 28
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.right: docs.left
        anchors.rightMargin: 16

        text: qsTr("Video")

        onClicked: videoButtonClicked()
    }
    */
    // Temporary placeholder for video button.
    Rectangle {
      id: video
      visible: false
    }

    RalewayButton {
        id: docs
        visible: false
        size: 28
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.right: parent.right
        anchors.rightMargin: 16

        text: qsTr("Docs")

        onClicked: docsButtonClicked()
    }
}
