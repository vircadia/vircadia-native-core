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

    visible: true
    
    anchors.left: parent.left
    anchors.right: parent.right
    anchors.top: parent.top
    anchors.bottom: parent.bottom

    RalewaySemiBold {
        id: avatarProjectLabel
        size: 24;
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.topMargin: 25
        anchors.bottomMargin: 25
        text: 'Avatar Project'
    }
    HifiControls.Button {
        id: openFolderButton
        anchors.left: parent.left
        anchors.right: parent.right
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
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: openFolderButton.bottom
        text: qsTr("Upload")
        color: hifi.buttons.blue
        colorScheme: root.colorScheme
        height: 30
        onClicked: function() {
                
        }
    }
}
