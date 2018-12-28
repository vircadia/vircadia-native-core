import QtQuick 2.6
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3
import QtQml.Models 2.1
import QtGraphicalEffects 1.0
import "../controlsUit" 1.0 as HifiControls
import "../stylesUit" 1.0
import "../windows" as Windows
import "../dialogs"
import "./avatarPackager" 1.0
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

        InfoBox {
            id: fileListPopup

            title: "List of Files"

            content: Rectangle {
                id: fileList

                color: "#404040"

                anchors.fill: parent
                anchors.topMargin: 10
                anchors.bottomMargin: 10
                anchors.leftMargin: 29
                anchors.rightMargin: 29

                clip: true

                ListView {
                    anchors.fill: parent
                    model: AvatarPackagerCore.currentAvatarProject === null ? [] : AvatarPackagerCore.currentAvatarProject.projectFiles
                    delegate: Rectangle {
                        width: parent.width
                        height: fileText.implicitHeight + 8
                        color: "#404040"
                        RalewaySemiBold {
                            id: fileText
                            size: 16
                            elide: Text.ElideLeft
                            anchors.top: parent.top
                            anchors.left: parent.left
                            anchors.right: parent.right
                            anchors.leftMargin: 16
                            anchors.rightMargin: 16
                            anchors.topMargin: 4
                            width: parent.width - 10
                            color: "white"
                            text: modelData
                        }
                    }
                }
            }
        }

        Rectangle {
            id: modalOverlay
            anchors.fill: parent
            z: 20
            color: "#a15d5d5d"
            visible: false

            // This mouse area captures the cursor events while the modalOverlay is active
            MouseArea {
                anchors.fill: parent
                propagateComposedEvents: false;
                hoverEnabled: true;
            }
        }

        AvatarApp.MessageBox {
            id: popup
            anchors.fill: parent
            visible: false
            closeOnClickOutside: true
        }

        Column {
            id: avatarPackager
            anchors.fill: parent
            state: "main"
            states: [
                State {
                    name: AvatarPackagerState.main
                    PropertyChanges { target: avatarPackagerHeader; title: qsTr("Avatar Packager"); faqEnabled: true; backButtonEnabled: false }
                    PropertyChanges { target: avatarPackagerMain; visible: true }
                    PropertyChanges { target: avatarPackagerFooter; content: avatarPackagerMain.footer }
                },
                State {
                    name: AvatarPackagerState.createProject
                    PropertyChanges { target: avatarPackagerHeader; title: qsTr("Create Project") }
                    PropertyChanges { target: createAvatarProject; visible: true }
                    PropertyChanges { target: avatarPackagerFooter; content: createAvatarProject.footer }
                },
                State {
                    name: AvatarPackagerState.project
                    PropertyChanges { target: avatarPackagerHeader; title: AvatarPackagerCore.currentAvatarProject.name; canRename: true }
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

            property alias showModalOverlay: modalOverlay.visible

            function openProject(path) {
                let project = AvatarPackagerCore.openAvatarProject(path);
                if (project) {
                    avatarProject.reset();
                    avatarPackager.state = "project";
                }
                return project;
            }

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
                                createAvatarProject.clearInputs();
                                avatarPackager.state = AvatarPackagerState.createProject;
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
                                avatarPackager.showModalOverlay = true;
                                let browser = desktop.fileDialog({
                                    selectDirectory: false,
                                    dir: fileDialogHelper.pathToUrl(AvatarPackagerCore.AVATAR_PROJECTS_PATH),
                                    filter: "Avatar Project FST Files (*.fst)",
                                    title: "Open Project (.fst)",
                                });

                                browser.canceled.connect(function() {
                                    avatarPackager.showModalOverlay = false;
                                });

                                browser.selectedFile.connect(function(fileUrl) {
                                    let fstFilePath = fileDialogHelper.urlToPath(fileUrl);
                                    let currentAvatarProject = avatarPackager.openProject(fstFilePath);
                                    if (currentAvatarProject) {
                                        avatarPackager.showModalOverlay = false;
                                    }
                                });
                            }
                        }
                    }

                    Flow {
                        visible: AvatarPackagerCore.recentProjects.length === 0
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

                    Column {
                        visible: AvatarPackagerCore.recentProjects.length > 0
                        anchors {
                            fill: parent
                            topMargin: 18
                            leftMargin: 16
                            rightMargin: 16
                        }
                        spacing: 10

                        Repeater {
                            model: AvatarPackagerCore.recentProjects
                            AvatarProjectCard {
                                title: modelData.name
                                path: modelData.path
                                onOpen: avatarPackager.openProject(modelData.path)
                            }
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
