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
import "avatarapp" 1.0 as AvatarApp

Windows.ScrollingWindow {
    id: root
    objectName: "AvatarPackager"
    width: 480
    height: 706
    title: "Avatar Packager"
    resizable: false
    opacity: parent.opacity
    destroyOnHidden: true
    implicitWidth: 384; implicitHeight: 640
    minSize: Qt.vector2d(480, 706)

    HifiConstants { id: hifi }

    Item {
        id: windowContent
        height: pane.height
        width: pane.width

        AvatarApp.MessageBox {
            id: popup
            anchors.fill: parent
            visible: false
        }

        // FIXME: modal overlay does not show
        Column {
            id: avatarPackager
            anchors.fill: parent
            state: "main"
            states: [
                State {
                    name: "main"
                    PropertyChanges { target: avatarPackagerHeader; title: qsTr("Avatar Packager"); faqEnabled: true; backButtonEnabled: false }
                    PropertyChanges { target: avatarPackagerMain; visible: true }
                    PropertyChanges { target: avatarPackagerFooter; content: avatarPackagerMain.footer }
                },
                State {
                    name: "createProject"
                    PropertyChanges { target: avatarPackagerHeader; title: qsTr("Create Project") }
                    PropertyChanges { target: createAvatarProject; visible: true }
                    PropertyChanges { target: avatarPackagerFooter; content: createAvatarProject.footer }
                },
                State {
                    name: "project"
                    PropertyChanges { target: avatarPackagerHeader; title: AvatarPackagerCore.currentAvatarProject.name }
                    PropertyChanges { target: avatarProject; visible: true }
                    PropertyChanges { target: avatarPackagerFooter; content: avatarProject.footer }
                },
                State {
                    name: "project-upload"
                    PropertyChanges { target: avatarPackagerHeader; title: AvatarPackagerCore.currentAvatarProject.name }
                    PropertyChanges { target: avatarUploader; visible: true }
                    PropertyChanges { target: avatarPackagerFooter; visible: false }
                }
            ]

            AvatarPackagerHeader {
                id: avatarPackagerHeader
                onBackButtonClicked: {
                    avatarPackager.state = "main"
                }
            }

            Item {
                height: pane.height - avatarPackagerHeader.height - avatarPackagerFooter.height
                width: pane.width

                Rectangle {
                    anchors.fill: parent
                    color: "#404040"
                }

                AvatarProject {
                    id: avatarProject
                    colorScheme: root.colorScheme
                    anchors.fill: parent
                }

                AvatarProjectUpload {
                    id: avatarUploader
                    anchors.fill: parent
                    root: avatarProject
                }

                CreateAvatarProject {
                    id: createAvatarProject
                    colorScheme: root.colorScheme
                    anchors.fill: parent
                }

                Item {
                    id: avatarPackagerMain
                    visible: false
                    anchors.fill: parent

                    property var footer: Item {
                        anchors.fill: parent
                        anchors.rightMargin: 17
                        HifiControls.Button {
                            id: createProjectButton
                            anchors.verticalCenter: parent.verticalCenter
                            anchors.right: openProjectButton.left
                            anchors.rightMargin: 22
                            height: 40
                            width: 134
                            text: qsTr("New Project")
                            colorScheme: root.colorScheme
                            onClicked: {
                                avatarPackager.state = "createProject";
                            }
                        }

                        HifiControls.Button {
                            id: openProjectButton
                            anchors.verticalCenter: parent.verticalCenter
                            anchors.right: parent.right
                            height: 40
                            width: 133
                            text: qsTr("Open Project")
                            color: hifi.buttons.blue
                            colorScheme: root.colorScheme
                            onClicked: {
                                // TODO: make the dialog modal
                                let browser = desktop.fileDialog({
                                    selectDirectory: false,
                                    dir: fileDialogHelper.pathToUrl(AvatarPackagerCore.AVATAR_PROJECTS_PATH),
                                    filter: "Avatar Project FST Files (*.fst)",
                                    title: "Open Project (.fst)",
                                });

                                browser.canceled.connect(function() {

                                });

                                browser.selectedFile.connect(function(fileUrl) {
                                    let fstFilePath = fileDialogHelper.urlToPath(fileUrl);
                                    let currentAvatarProject = AvatarPackagerCore.openAvatarProject(fstFilePath);
                                    if (currentAvatarProject) {
                                        avatarPackager.state = "project";
                                    }
                                });
                            }
                        }
                    }
                    Flow {
                        anchors {
                            fill: parent
                            topMargin: 18
                            leftMargin: 16
                            rightMargin: 16
                        }
                        RalewayRegular {
                            size: 20
                            color: "white"
                            text: qsTr("Use a custom avatar to express your identity")
                        }
                        RalewayRegular {
                            size: 20
                            color: "white"
                            text: qsTr("To learn more about using this tool, visit our docs")
                        }
                    }
                }
            }
            AvatarPackagerFooter {
                id: avatarPackagerFooter
            }
        }
    }
}
