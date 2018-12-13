import QtQuick 2.6
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3
import QtQml.Models 2.1
import QtGraphicalEffects 1.0
import "../controlsUit" 1.0 as HifiControls
import "../stylesUit" 1.0
import "../windows" as Windows
import "../dialogs"
import "avatarPackager"

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

    //HifiConstants { id: hifi }
    Rectangle {
        anchors.fill: parent

        AvatarProject {
            id: avatarProject
            colorScheme: root.colorScheme
            visible: false
            anchors.fill: parent
        }

        Rectangle {
            id: avatarPackagerMain
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.bottom: parent.bottom
            RalewaySemiBold {
                id: avatarPackagerLabel
                size: 24;
                anchors.left: parent.left
                anchors.top: parent.top
                anchors.topMargin: 25
                anchors.bottomMargin: 25
                text: 'Avatar Packager'
            }

            HifiControls.Button {
                id: createProjectButton
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: avatarPackagerLabel.bottom
                text: qsTr("Create Project")
                colorScheme: root.colorScheme
                height: 30
                onClicked: function() {
                
                }
            }
            HifiControls.Button {
                id: openProjectButton
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: createProjectButton.bottom
                text: qsTr("Open Avatar Project")
                colorScheme: root.colorScheme
                height: 30
                onClicked: function() {
                    var avatarProjectsPath = fileDialogHelper.standardPath(/*fileDialogHelper.StandardLocation.DocumentsLocation*/ 1) + "/High Fidelity/Avatar Projects";
                    console.log("path = " + avatarProjectsPath);

                    // TODO: make the dialog modal

                    var browser = desktop.fileDialog({
                        selectDirectory: false,
                        dir: fileDialogHelper.pathToUrl(avatarProjectsPath),
                        filter: "Avatar Project FST Files (*.fst)",
                        title: "Open Project (.fst)"
                    });

                    browser.canceled.connect(function() {
                    
                    });

                    browser.selectedFile.connect(function(fileUrl) {
                        console.log("FOUND PATH " + fileUrl);
                        let fstFilePath = fileDialogHelper.urlToPath(fileUrl);
                        let currentAvatarProject = AvatarPackagerCore.openAvatarProject(fstFilePath);
                        if (currentAvatarProject) {
                            console.log("LOAD COMPLETE");
                            console.log("file dir = " + AvatarPackagerCore.currentAvatarProject.projectFolderPath);
                            
                            avatarPackagerMain.visible = false;
                            avatarProject.visible = true;
                        }
                    });
                }
            }
        }
    }
}
