import QtQuick 2.6
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3

import "../../controlsUit" 1.0 as HifiControls
import "../../stylesUit" 1.0

Item {
    id: root

    HifiConstants { id: hifi }

    property int colorScheme

    property var footer: Item {
        anchors.fill: parent
        anchors.rightMargin: 17
        HifiControls.Button {
            id: createButton
            anchors.verticalCenter: parent.verticalCenter
            anchors.right: parent.right
            height: 30
            width: 133
            text: qsTr("Create")
            enabled: false
            onClicked: {
                if (!AvatarPackagerCore.createAvatarProject(projectLocation.text, name.text, avatarModel.text, textureFolder.text)) {
                    Window.alert('Failed to create project');
                    return;
                }
                avatarProject.reset();
                avatarPackager.state = AvatarPackagerState.project;
            }
        }
    }

    visible: false
    anchors.fill: parent
    height: parent.height
    width: parent.width

    function clearInputs() {
        name.text = projectLocation.text = avatarModel.text = textureFolder.text = "";
    }

    function checkErrors() {
        let newErrorMessageText = "";

        let projectName = name.text;
        let projectFolder = projectLocation.text;

        let hasProjectNameError = projectName !== "" && projectFolder !== "" && !AvatarPackagerCore.isValidNewProjectName(projectFolder, projectName);

        if (hasProjectNameError) {
            newErrorMessageText =  "A folder with that name already exists at that location. Please choose a different project name or location.";
        }

        name.error = projectLocation.error = hasProjectNameError;
        errorMessage.text = newErrorMessageText;
        createButton.enabled = newErrorMessageText === "" && requiredFieldsFilledIn();
    }

    function requiredFieldsFilledIn() {
        return name.text !== "" && projectLocation.text !== "" && avatarModel.text !== "";
    }

    RalewayRegular {
        id: errorMessage
        visible: text !== ""
        text: ""
        color: "#EA4C5F"
        wrapMode: Text.WordWrap
        size: 20
        anchors {
            left: parent.left
            right: parent.right
        }
    }

    Column {
        id: createAvatarColumns
        anchors.top: errorMessage.visible ? errorMessage.bottom : parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.margins: 10

        spacing: 17

        property string defaultFileBrowserPath: fileDialogHelper.pathToUrl(AvatarPackagerCore.AVATAR_PROJECTS_PATH)

        ProjectInputControl {
            id: name
            label: "Name"
            colorScheme: root.colorScheme
            onTextChanged: checkErrors()
        }

        ProjectInputControl {
            id: projectLocation
            label: "Specify Project Location"
            colorScheme: root.colorScheme
            browseEnabled: true
            browseFolder: true
            browseDir: text !== "" ? fileDialogHelper.pathToUrl(text) : createAvatarColumns.defaultFileBrowserPath
            browseTitle: "Project Location"
            onTextChanged: checkErrors()
        }

        ProjectInputControl {
            id: avatarModel
            label: "Specify Avatar Model (.fbx)"
            colorScheme: root.colorScheme
            browseEnabled: true
            browseFolder: false
            browseDir: text !== "" ? fileDialogHelper.pathToUrl(text) : createAvatarColumns.defaultFileBrowserPath
            browseFilter: "Avatar Model File (*.fbx)"
            browseTitle: "Open Avatar Model (.fbx)"
            onTextChanged: checkErrors()
        }

        ProjectInputControl {
            id: textureFolder
            label: "Specify Texture Folder - <i>Optional</i>"
            colorScheme: root.colorScheme
            browseEnabled: true
            browseFolder: true
            browseDir: text !== "" ? fileDialogHelper.pathToUrl(text) : createAvatarColumns.defaultFileBrowserPath
            browseTitle: "Texture Folder"
            onTextChanged: checkErrors()
        }
    }
}
