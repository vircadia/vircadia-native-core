import QtQuick 2.6
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3
import QtGraphicalEffects 1.0

import QtQuick.Controls 2.2 as Original

import "../../controlsUit" 1.0 as HifiControls
import "../../stylesUit" 1.0


Item {
    id: root

    HifiConstants { id: hifi }

    Style { id: style }

    property int colorScheme;
    property var uploader: undefined;

    property bool hasSuccessfullyUploaded: true;

    visible: false
    anchors.fill: parent
    anchors.margins: 10

    function reset() { hasSuccessfullyUploaded = false }

    property var footer: Item {
        anchors.fill: parent

        Item {
            id: uploadFooter

            visible: !root.uploader || root.finished || root.uploader.state !== 4

            anchors.fill: parent
            anchors.rightMargin: 17

            HifiControls.Button {
                id: uploadButton

                visible: !AvatarPackagerCore.currentAvatarProject.fst.hasMarketplaceID && !root.hasSuccessfullyUploaded
                enabled: Account.loggedIn

                anchors.verticalCenter: parent.verticalCenter
                anchors.right: parent.right

                text: qsTr("Upload")
                color: hifi.buttons.blue
                colorScheme: root.colorScheme
                width: 133
                height: 40
                onClicked: {
                    uploadNew();
                }
            }
            HifiControls.Button {
                id: updateButton

                visible: AvatarPackagerCore.currentAvatarProject.fst.hasMarketplaceID && !root.hasSuccessfullyUploaded
                enabled: Account.loggedIn

                anchors.verticalCenter: parent.verticalCenter
                anchors.right: parent.right

                text: qsTr("Update")
                color: hifi.buttons.blue
                colorScheme: root.colorScheme
                width: 134
                height: 40
                onClicked: {
                    showConfirmUploadPopup(uploadNew, uploadUpdate);
                }
            }
            Item {
                anchors.fill: parent
                visible: root.hasSuccessfullyUploaded

                HifiControls.Button {
                    enabled: Account.loggedIn

                    anchors.verticalCenter: parent.verticalCenter
                    anchors.right: viewInInventoryButton.left
                    anchors.rightMargin: 16

                    text: qsTr("Update")
                    color: hifi.buttons.white
                    colorScheme: root.colorScheme
                    width: 134
                    height: 40
                    onClicked: {
                        showConfirmUploadPopup(uploadNew, uploadUpdate);
                    }
                }
                HifiControls.Button {
                    id: viewInInventoryButton

                    enabled: Account.loggedIn

                    width: 168
                    height: 40

                    anchors.verticalCenter: parent.verticalCenter
                    anchors.right: parent.right

                    text: qsTr("View in Inventory")
                    color: hifi.buttons.blue
                    colorScheme: root.colorScheme

                    onClicked: AvatarPackagerCore.currentAvatarProject.openInInventory()
                }
            }
        }

        Rectangle {
            id: uploadingItemFooter

            anchors.fill: parent
            anchors.topMargin: 1 
            visible: !!root.uploader && !root.finished && root.uploader.state === 4

            color: "#00B4EF"

            LoadingCircle {
                id: runningImage

                anchors.verticalCenter: parent.verticalCenter
                anchors.left: parent.left
                anchors.leftMargin: 16

                width: 28
                height: 28

                white: true
            }
            RalewayRegular {
                id: stepText

                size: 20

                anchors.verticalCenter: parent.verticalCenter
                anchors.left: runningImage.right
                anchors.leftMargin: 16

                text: "Adding item to Inventory"
                color: "white"
            }
        }
    }

    function uploadNew() {
        console.log("Uploading new");
        upload(false);
    }
    function uploadUpdate() {
        console.log("Uploading update");
        upload(true);
    }

    function upload(updateExisting) {
        root.uploader = AvatarPackagerCore.currentAvatarProject.upload(updateExisting);
        console.log("uploader: "+ root.uploader);
        root.uploader.uploadProgress.connect(function(uploaded, total) {
            console.log("Uploader progress: " + uploaded + " / " + total);
        });
        root.uploader.completed.connect(function() {
            root.hasSuccessfullyUploaded = true;
        });
        root.uploader.finishedChanged.connect(function() {
            try {
                var response = JSON.parse(root.uploader.responseData);
                console.log("Uploader complete! " + response);
                uploadStatus.text = response.status;
            } catch (e) {
                console.log("Error parsing JSON: " + root.uploader.reponseData);
            }
        });
        root.uploader.send();
        avatarPackager.state = AvatarPackagerState.projectUpload;
    }

    function showConfirmUploadPopup() {
        popup.titleText = 'Overwrite Avatar'
        popup.bodyText = 'You have previously uploaded the avatar file from this project.' + 
                         ' This will overwrite that avatar and you won’t be able to access the older version.'

        popup.button1text = 'CREATE NEW';
        popup.button2text = 'OVERWRITE';

        popup.onButton2Clicked = function() {
            popup.close();
            uploadUpdate();
        }
        popup.onButton1Clicked = function() {
            popup.close();
            showConfirmCreateNewPopup();
        };

        popup.open();
    }

    function showConfirmCreateNewPopup(confirmCallback) {
        popup.titleText = 'Create New'
        popup.bodyText = 'This will upload your current files with the same avatar name.' +
                         ' You will lose the ability to update the previously uploaded avatar. Are you sure you want to continue?'

        popup.button1text = 'CANCEL';
        popup.button2text = 'CONFIRM';

        popup.onButton1Clicked = function() {
            popup.close()
        };
        popup.onButton2Clicked = function() {
            popup.close();
            uploadNew();
        };

        popup.open();
    }

    RalewayRegular {
        id: infoMessage

        states: [
            State {
                when: root.hasSuccessfullyUploaded
                name: "upload-success"
                PropertyChanges {
                    target: infoMessage
                    text: "Your avatar has been successfully uploaded to our servers. Make changes to your avatar by editing and uploading the project files."
                }
            },
            State {
                name: "has-previous-success"
                when: !!AvatarPackagerCore.currentAvatarProject && AvatarPackagerCore.currentAvatarProject.fst.hasMarketplaceID
                PropertyChanges {
                    target: infoMessage
                    text: "Click \"Update\" to overwrite the hosted files and update the avatar in your inventory. You will have to “Wear” the avatar again to see changes."
                }
            }
        ]

        color: 'white'
        size: 20

        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top

        anchors.bottomMargin: 24

        wrapMode: Text.Wrap

        text: "You can upload your files to our servers to always access them, and to make your avatar visible to other users."
    }

    HifiControls.Button {
        id: openFolderButton

        visible: false
        width: parent.width
        anchors.top: infoMessage.bottom
        anchors.topMargin: 10
        text: qsTr("Open Project Folder")
        colorScheme: root.colorScheme
        height: 30
        onClicked: {
            fileDialogHelper.openDirectory(fileDialogHelper.pathToUrl(AvatarPackagerCore.currentAvatarProject.projectFolderPath));
        }
    }

    RalewayRegular {
        id: showFilesText

        color: 'white'
        linkColor: style.colors.blueHighlight

        visible: AvatarPackagerCore.currentAvatarProject !== null

        anchors.bottom: loginRequiredMessage.top
        anchors.bottomMargin: 10

        size: 20

        text: AvatarPackagerCore.currentAvatarProject.projectFiles.length + " files in project. <a href='toggle'>See list</a>"

        onLinkActivated: fileListPopup.open()
    }

    Rectangle {
        id: loginRequiredMessage

        visible: !Account.loggedIn
        height: !Account.loggedIn ? loginRequiredTextRow.height + 20 : 0

        anchors {
            bottom: parent.bottom
            left: parent.left
            right: parent.right
        }

        color: "#FFD6AD"

        border.color: "#F39622"
        border.width: 2
        radius: 2

        Item {
            id: loginRequiredTextRow

            height: Math.max(loginWarningGlyph.implicitHeight, loginWarningText.implicitHeight)
            anchors.fill: parent
            anchors.margins: 10

            HiFiGlyphs {
                id: loginWarningGlyph

                anchors.verticalCenter: parent.verticalCenter
                anchors.left: parent.left

                width: implicitWidth

                size: 48
                text: "+"
                color: "black"
            }
            RalewayRegular {
                id: loginWarningText

                anchors.verticalCenter: parent.verticalCenter
                anchors.leftMargin: 16
                anchors.left: loginWarningGlyph.right
                anchors.right: parent.right

                text: "Please login to upload your avatar to High Fidelity hosting."
                size: 18
                wrapMode: Text.Wrap
            }
        }
    }
}
