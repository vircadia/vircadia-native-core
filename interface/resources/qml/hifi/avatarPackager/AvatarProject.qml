import QtQuick 2.6
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3
import QtGraphicalEffects 1.0

import "../../controlsUit" 1.0 as HifiControls
import "../../stylesUit" 1.0

Rectangle {
    id: root

    HifiConstants { id: hifi }

    property int colorScheme;

    color: "blue"

    visible: true

    anchors.fill: parent

    RalewaySemiBold {
        id: avatarProjectLabel
        size: 24;
        width: parent.width
        anchors.topMargin: 25
        anchors.bottomMargin: 25
        text: 'Avatar Project'
    }
    HifiControls.Button {
        id: openFolderButton
        width: parent.width
        anchors.top: avatarProjectLabel.bottom
        text: qsTr("Open Project Folder")
        colorScheme: root.colorScheme
        height: 30
        onClicked: function() {
            fileDialogHelper.openDirectory(AvatarPackagerCore.currentAvatarProject.projectFolderPath);
        }
    }
    HifiControls.Button {
        id: uploadButton
        width: parent.width
        anchors.top: openFolderButton.bottom
        text: qsTr("Upload")
        color: hifi.buttons.blue
        colorScheme: root.colorScheme
        height: 30
        onClicked: function() {
        }
    }
    Text {
        id: modelText
        anchors.top: uploadButton.bottom
        height: 30
        text: parent.height
    }
    Rectangle {
        color: 'white'
        visible: AvatarPackagerCore.currentAvatarProject !== null
        width: parent.width
        anchors.top: modelText.bottom
        height: 1000

        ListView {
            anchors.fill: parent
            model: AvatarPackagerCore.currentAvatarProject === null ? [] : AvatarPackagerCore.currentAvatarProject.projectFiles
            delegate: Text { text: '<b>File:</b> ' + modelData }
        }
    }
}
