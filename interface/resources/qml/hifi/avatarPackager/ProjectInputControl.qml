import QtQuick 2.6

import "../../controlsUit" 1.0 as HifiControls
import "../../stylesUit" 1.0

Column {
    id: control

    anchors.left: parent.left
    anchors.leftMargin: 21
    anchors.right: parent.right
    anchors.rightMargin: 16

    height: 75

    spacing: 4

    property alias label: label.text
    property alias browseEnabled: browseButton.visible
    property bool browseFolder: false
    property string browseFilter: "All Files (*.*)"
    property string browseTitle: "Open file"
    property string browseDir: ""
    property alias text: input.text
    property alias error: input.error

    property int colorScheme

    Row {
        RalewaySemiBold {
            id: label
            size: 20
            font.weight: Font.Medium
            text: ""
            color: "white"
        }
    }
    Row {
        width: control.width
        spacing: 16
        height: 40
        HifiControls.TextField {
            id: input
            colorScheme: control.colorScheme
            font.family: "Fira Sans"
            font.pixelSize: 18
            height: parent.height
            width: browseButton.visible ? parent.width - browseButton.width - parent.spacing : parent.width
        }

        HifiControls.Button {
            id: browseButton
            visible: false
            height: parent.height
            width: 133
            text: qsTr("Browse")
            colorScheme: root.colorScheme
            onClicked: {
                avatarPackager.showModalOverlay = true;
                let browser = avatarPackager.desktopObject.fileDialog({
                     selectDirectory: browseFolder,
                     dir: browseDir,
                     filter: browseFilter,
                     title: browseTitle,
                });

                browser.canceled.connect(function() {
                    avatarPackager.showModalOverlay = false;
                });

                browser.selectedFile.connect(function(fileUrl) {
                    input.text = fileDialogHelper.urlToPath(fileUrl);
                    avatarPackager.showModalOverlay = false;
                });
            }
        }
    }
}
