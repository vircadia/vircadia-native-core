import QtQuick 2.6
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3
import QtGraphicalEffects 1.0

import QtQuick.Controls 2.2 as Original

import "../../controlsUit" 1.0 as HifiControls
import "../../stylesUit" 1.0

Item {
    id: root

    HifiConstants { id: hifi }

    property int colorScheme;
    property var uploader: undefined;

    visible: true

    anchors.fill: parent
    anchors.margins: 10

    RalewaySemiBold {
        id: avatarProjectLabel
        size: 24;
        width: parent.width
        anchors.topMargin: 25
        anchors.bottomMargin: 25
        text: 'Avatar Project'
        color: "white"
    }
    HifiControls.Button {
        id: openFolderButton
        width: parent.width
        anchors.top: avatarProjectLabel.bottom
        anchors.topMargin: 10
        text: qsTr("Open Project Folder")
        colorScheme: root.colorScheme
        height: 30
        onClicked: function() {
            fileDialogHelper.openDirectory(AvatarPackagerCore.currentAvatarProject.projectFolderPath);
        }
    }
    Rectangle {
        color: 'white'
        visible: AvatarPackagerCore.currentAvatarProject !== null
        anchors.top: openFolderButton.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: uploadButton.top
        anchors.topMargin: 10
        anchors.bottomMargin: 10
        height: 1000

        ListView {
            anchors.fill: parent
            model: AvatarPackagerCore.currentAvatarProject === null ? [] : AvatarPackagerCore.currentAvatarProject.projectFiles
            delegate: Text { text: '<b>File:</b> ' + modelData }
        }
    }
    HifiControls.Button {
        id: uploadButton

        width: parent.width
        height: 30
        anchors.bottom: parent.bottom

        text: qsTr("Upload")
        color: hifi.buttons.blue
        colorScheme: root.colorScheme
        onClicked: function() {
            console.log("Uploading");
            parent.uploader = AvatarPackagerCore.currentAvatarProject.upload();
            console.log("uploader: "+ parent.uploader);
            parent.uploader.uploadProgress.connect(function(uploaded, total) {
                console.log("Uploader progress: " + uploaded + " / " + total);
            });
            parent.uploader.completed.connect(function() {
                try {
                    var response = JSON.parse(parent.uploader.responseData);
                    console.log("Uploader complete! " + response);
                    uploadStatus.text = response.status;
                } catch (e) {
                    console.log("Error parsing JSON: " + parent.uploader.reponseData);
                }
            });
            parent.uploader.send();
        }
    }

    Rectangle {
        id: uploadingScreen

        visible: !!root.uploader
        anchors.fill: parent

        color: "black"

        Item {
            visible: !!root.uploader && !root.uploader.complete

            anchors.fill: parent

            AnimatedImage {
                id: uploadSpinner

                anchors {
                    horizontalCenter: parent.horizontalCenter
                    verticalCenter: parent.verticalCenter
                }

                source: "../../../icons/loader-snake-64-w.gif"
                playing: true
                z: 10000
            }
        }

        Item {
            visible: !!root.uploader && root.uploader.complete

            anchors.fill: parent

            HiFiGlyphs {
                id: successIcon

                anchors {
                    horizontalCenter: parent.horizontalCenter
                    verticalCenter: parent.verticalCenter
                }

                size: 128
                text: "\ue01a"
                color: "#1FC6A6"
            }

            Text {
                text: "Congratulations!"

                anchors.horizontalCenter: parent.horizontalCenter
                anchors.top: successIcon.bottom

                color: "white"
            }

            HifiControls.Button {
                width: implicitWidth
                height: implicitHeight

                anchors.bottom: parent.bottom
                anchors.right: parent.right

                text: "View in Inventory"

                color: hifi.buttons.blue
                colorScheme: root.colorScheme
                onClicked: function() {
                    console.log("Opening in inventory");
                }
            }
        }

        Column {
            Text {
                id: uploadStatus

                text: "Uploading"
                color: "white"

            }
            Text {
                text: "State: " + (!!root.uploader ? root.uploader.state : " NONE")
                color: "white"
            }
        }

    }
}
