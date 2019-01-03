import QtQuick 2.6
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3
import QtQml.Models 2.1
import QtGraphicalEffects 1.0
import "../../controlsUit" 1.0 as HifiControls
import "../../stylesUit" 1.0
import "../../windows" as Windows
import "../../controls" 1.0
import "../../dialogs"
import "../avatarapp" 1.0 as AvatarApp

Item {
    HifiConstants { id: hifi }

    property alias desktopObject: avatarPackager.desktopObject

    id: windowContent
   // height: pane ? pane.height : parent.width
   // width: pane ? pane.width : parent.width


    MouseArea {
        anchors.fill: parent

        onClicked: {
            unfocusser.forceActiveFocus();
        }
        Item {
            id: unfocusser
            visible: false
        }
    }

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
                PropertyChanges { target: avatarPackagerHeader; title: qsTr("Avatar Packager"); docsEnabled: true; backButtonVisible: false }
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
                name: AvatarPackagerState.projectUpload
                PropertyChanges { target: avatarPackagerHeader; title: AvatarPackagerCore.currentAvatarProject.name; backButtonEnabled: false }
                PropertyChanges { target: avatarUploader; visible: true }
                PropertyChanges { target: avatarPackagerFooter; visible: false }
            }
        ]

        property alias showModalOverlay: modalOverlay.visible

        property var desktopObject: desktop

        function openProject(path) {
            let project = AvatarPackagerCore.openAvatarProject(path);
            if (project) {
                avatarProject.reset();
                avatarPackager.state = AvatarPackagerState.project;
            }
            return project;
        }

        function openDocs() {
            Qt.openUrlExternally("https://docs.highfidelity.com/create/avatars/create-avatars#how-to-package-your-avatar");
        }

        AvatarPackagerHeader {
            z: 100

            id: avatarPackagerHeader
            colorScheme: root.colorScheme
            onBackButtonClicked: {
                avatarPackager.state = AvatarPackagerState.main;
            }
            onDocsButtonClicked: {
                avatarPackager.openDocs();
            }
        }

        Item {
            height: windowContent.height - avatarPackagerHeader.height - avatarPackagerFooter.height
            width: windowContent.width

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

                            let browser = avatarPackager.desktopObject.fileDialog({
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

                Item {
                    anchors.fill: parent

                    visible: AvatarPackagerCore.recentProjects.length > 0

                    RalewayRegular {
                        id: recentProjectsText

                        color: 'white'

                        anchors.top: parent.top
                        anchors.left: parent.left
                        anchors.topMargin: 16
                        anchors.leftMargin: 16

                        size: 20

                        text: "Recent Projects"

                        onLinkActivated: fileListPopup.open()
                    }

                    Column {
                        anchors {
                            left: parent.left
                            right: parent.right
                            bottom: parent.bottom
                            top: recentProjectsText.bottom
                            topMargin: 16
                            leftMargin: 16
                            rightMargin: 16
                        }
                        spacing: 10

                        Repeater {
                            model: AvatarPackagerCore.recentProjects
                            AvatarProjectCard {
                                title: modelData.name
                                path: modelData.projectPath
                                onOpen: avatarPackager.openProject(modelData.path)
                            }
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
