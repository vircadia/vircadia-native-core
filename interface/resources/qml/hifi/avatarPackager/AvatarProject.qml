import QtQuick 2.6
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3
import QtGraphicalEffects 1.0

import "../../controlsUit" 1.0 as HifiControls
import "../../stylesUit" 1.0

Item {
    id: root

    HifiConstants { id: hifi }

    property int colorScheme;

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
        anchors.bottom: parent.bottom
        text: qsTr("Upload")
        color: hifi.buttons.blue
        colorScheme: root.colorScheme
        height: 30
        onClicked: function() {
        }
    }
}
