import QtQuick 2.6
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3
import QtQml.Models 2.1
import QtGraphicalEffects 1.0
import "../controlsUit" 1.0 as HifiControls
import "../stylesUit" 1.0
import "../windows" as Windows
import "../dialogs"

Windows.ScrollingWindow {
    id: root
    objectName: "AvatarPackager"
    width: 480
    height: 706
    title: "Avatar Packager"
    resizable: true
    opacity: parent.opacity
    destroyOnHidden: true
    implicitWidth: 384; implicitHeight: 640
    minSize: Qt.vector2d(200, 300)
    Rectangle {
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        RalewaySemiBold {
            id: displayNameLabel
            size: 24;
            anchors.left: parent.left
            anchors.top: parent.top
            anchors.topMargin: 25
            text: 'Avatar Projects'
        }

        HifiControls.Button {
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: displayNameLabel.bottom
            text: qsTr("Open Avatar Project")
           // color: hifi.buttons.blue
            colorScheme: root.colorScheme
            height: 30
            onClicked: function() {
                let avatarProject = AvatarPackagerCore.openAvatarProject();
                if (avatarProject) {
                    console.log("LOAD COMPLETE");
                } else {
                    console.log("LOAD FAILED");
                }
            }
        }
    }
}
