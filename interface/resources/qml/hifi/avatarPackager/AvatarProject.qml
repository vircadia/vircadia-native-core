import QtQuick 2.6
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3
import QtGraphicalEffects 1.0

import "../../controlsUit" 1.0 as HifiControls
import "../../stylesUit" 1.0

Item {
    id: root

    HifiConstants { id: hifi }

    property int colorScheme

    visible: false
    anchors.fill: parent
    anchors.margins: 10

    property var footer: Item {
        anchors.fill: parent
        anchors.rightMargin: 17
        HifiControls.Button {
            id: uploadButton
            //width: parent.width
            //anchors.bottom: parent.bottom
            anchors.verticalCenter: parent.verticalCenter
            anchors.right: parent.right
            text: qsTr("Upload")
            color: hifi.buttons.blue
            colorScheme: root.colorScheme
            width: 133
            height: 40
            onClicked: {
            }
        }
    }

    RalewaySemiBold {
        id: avatarFBXNameLabel
        size: 14
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.topMargin: 25
        anchors.bottomMargin: 25
        text: qsTr("FBX file here")
    }

    HifiControls.Button {
        id: openFolderButton
        width: parent.width
        anchors.top: avatarFBXNameLabel.bottom
        anchors.topMargin: 10
        text: qsTr("Open Project Folder")
        colorScheme: root.colorScheme
        height: 30
        onClicked: {
            fileDialogHelper.openDirectory(fileDialogHelper.pathToUrl(AvatarPackagerCore.currentAvatarProject.projectFolderPath));
        }
    }

    Rectangle {
        color: "white"
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
}
