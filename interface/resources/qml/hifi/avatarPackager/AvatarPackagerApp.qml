import QtQuick 2.6
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3
import QtQml.Models 2.1
import QtGraphicalEffects 1.0
import Hifi.AvatarPackager.AvatarProjectStatus 1.0
import "../../controlsUit" 1.0 as HifiControls
import "../../stylesUit" 1.0
import "../../controls" 1.0
import "../../dialogs"
import "../avatarapp" 1.0 as AvatarApp

Item {
    id: windowContent

    HifiConstants { id: hifi }

    property alias desktopObject: avatarPackager.desktopObject

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

    InfoBox {
        id: errorPopup

        property string errorMessage

        boxWidth: 380
        boxHeight: 293

        content: RalewayRegular {

            id: bodyMessage

            anchors.fill: parent
            anchors.bottomMargin: 10
            anchors.leftMargin: 29
            anchors.rightMargin: 29

            size: 20
            color: "white"
            text: errorPopup.errorMessage
            width: parent.width
            wrapMode: Text.WordWrap
        }

        function show(title, message) {
            errorPopup.title = title;
            errorMessage = message;
            errorPopup.open();
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
            propagateComposedEvents: false
            hoverEnabled: true
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
                PropertyChanges { target: avatarPackagerHeader; title: qsTr("Avatar Packager"); docsEnabled: true; videoEnabled: true; backButtonVisible: false }
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
                name: AvatarPackagerState.avatarDoctorDiagnose
                PropertyChanges { target: avatarPackagerHeader; title: AvatarPackagerCore.currentAvatarProject.name }
                PropertyChanges { target: avatarDoctorDiagnose; visible: true }
                PropertyChanges { target: avatarPackagerFooter; content: avatarDoctorDiagnose.footer }
            },
            State {
                name: AvatarPackagerState.avatarDoctorErrorReport
                PropertyChanges { target: avatarPackagerHeader; title: AvatarPackagerCore.currentAvatarProject.name }
                PropertyChanges { target: avatarDoctorErrorReport; visible: true }
                PropertyChanges { target: avatarPackagerFooter; content: avatarDoctorErrorReport.footer }
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
            let status = AvatarPackagerCore.openAvatarProject(path);
            if (status !== AvatarProjectStatus.SUCCESS) {
                displayErrorMessage(status);
                return status;
            }
            avatarProject.reset();
            avatarPackager.state = AvatarPackagerState.avatarDoctorDiagnose;
            return status;
        }

        function displayErrorMessage(status) {
            if (status === AvatarProjectStatus.SUCCESS) {
                return;
            }
            switch (status) {
                case AvatarProjectStatus.ERROR_CREATE_PROJECT_NAME:
                    errorPopup.show("Project Folder Already Exists", "A folder with that name already exists at that location. Please choose a different project name or location.");
                    break;
                case AvatarProjectStatus.ERROR_CREATE_CREATING_DIRECTORIES:
                    errorPopup.show("Project Folders Creation Error", "There was a problem creating the Avatar Project directory. Please check the project location and try again.");
                    break;
                case AvatarProjectStatus.ERROR_CREATE_FIND_MODEL:
                    errorPopup.show("Cannot Find Model File", "There was a problem while trying to find the specified model file. Please verify that it exists at the specified location.");
                    break;
                case AvatarProjectStatus.ERROR_CREATE_OPEN_MODEL:
                    errorPopup.show("Cannot Open Model File", "There was a problem while trying to open the specified model file.");
                    break;
                case AvatarProjectStatus.ERROR_CREATE_READ_MODEL:
                    errorPopup.show("Error Read Model File", "There was a problem while trying to read the specified model file. Please check that the file is a valid FBX file and try again.");
                    break;
                case AvatarProjectStatus.ERROR_CREATE_WRITE_FST:
                    errorPopup.show("Error Writing Project File", "There was a problem while trying to write the FST file.");
                    break;
                case AvatarProjectStatus.ERROR_OPEN_INVALID_FILE_TYPE:
                    errorPopup.show("Invalid Project Path", "The avatar packager can only open FST files.");
                    break;
                case AvatarProjectStatus.ERROR_OPEN_PROJECT_FOLDER:
                    errorPopup.show("Project Missing", "Project folder cannot be found. Please locate the folder and copy/move it to its original location.");
                    break;
                case AvatarProjectStatus.ERROR_OPEN_FIND_FST:
                    errorPopup.show("File Missing", "We cannot find the project file (.fst) in the project folder. Please locate it and move it to the project folder.");
                    break;
                case AvatarProjectStatus.ERROR_OPEN_OPEN_FST:
                    errorPopup.show("File Read Error", "We cannot read the project file (.fst).");
                    break;
                case AvatarProjectStatus.ERROR_OPEN_FIND_MODEL:
                    errorPopup.show("File Missing", "We cannot find the avatar model file (.fbx) in the project folder. Please locate it and move it to the project folder.");
                    break;
                default:
                    errorPopup.show("Error Message Missing", "Error message missing for status " + status);
            }

        }

        function openDocs() {
            Qt.openUrlExternally("https://docs.vircadia.com/create/avatars/package-avatar.html");
        }

        function openVideo() {
            Qt.openUrlExternally("https://youtu.be/zrkEowu_yps");
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
            onVideoButtonClicked: {
                avatarPackager.openVideo();
            }
        }

        Item {
            height: windowContent.height - avatarPackagerHeader.height - avatarPackagerFooter.height
            width: windowContent.width

            Rectangle {
                anchors.fill: parent
                color: "#404040"
            }

            AvatarDoctorDiagnose {
                id: avatarDoctorDiagnose
                anchors.fill: parent
                onErrorsChanged: {
                    avatarDoctorErrorReport.errors = avatarDoctorDiagnose.errors;
                }
                onDoneDiagnosing: {
                    avatarPackager.state = avatarDoctorDiagnose.errors.length > 0 ? AvatarPackagerState.avatarDoctorErrorReport
                                                                                  : AvatarPackagerState.project;
                }
            }

            AvatarDoctorErrorReport {
                id: avatarDoctorErrorReport
                anchors.fill: parent
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
                                avatarPackager.showModalOverlay = false;
                                avatarPackager.openProject(fstFilePath);
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
                        text: "Use a custom avatar of your choice."
                        width: parent.width
                        wrapMode: Text.WordWrap
                    }
                    RalewayRegular {
                        size: 20
                        color: "white"
                        text: "<a href='javascript:void'>Visit our docs</a> to learn more about using the packager."
                        linkColor: "#00B4EF"
                        width: parent.width
                        wrapMode: Text.WordWrap
                        onLinkActivated: {
                            avatarPackager.openDocs();
                        }
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
                                hasError: modelData.hadErrors
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
