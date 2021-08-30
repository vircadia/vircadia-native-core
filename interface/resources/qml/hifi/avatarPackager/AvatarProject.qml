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

    property int colorScheme
    property var uploader: null

    property bool hasSuccessfullyUploaded: true

    visible: false
    anchors.fill: parent
    anchors.margins: 10

    function reset() {
        hasSuccessfullyUploaded = false;
        uploader = null;
    }

    property var footer: Item {
        anchors.fill: parent

        Item {
            id: uploadFooter

            visible: !root.uploader || root.finished || root.uploader.state !== 4

            anchors.fill: parent
            anchors.rightMargin: 17

            HifiControls.Button {
                id: uploadButton

                // FIXME: Re-enable if ability to upload to hosted location is added.
                /*
                visible: AvatarPackagerCore.currentAvatarProject && !AvatarPackagerCore.currentAvatarProject.fst.hasMarketplaceID && !root.hasSuccessfullyUploaded
                */
                visible: false
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

                // FIXME: Re-enable if ability to upload to hosted location is added.
                /*
                visible: AvatarPackagerCore.currentAvatarProject && AvatarPackagerCore.currentAvatarProject.fst.hasMarketplaceID && !root.hasSuccessfullyUploaded
                */
                visible: false
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

                // FIXME: Re-enable if ability to upload to hosted location is added.
                /*
                visible: root.hasSuccessfullyUploaded
                */
                visible: false;

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
            // FIXME: Remove if "Upload" button is reinstated.
            HifiControls.Button {
                id: openDirectoryButton
                visible: AvatarPackagerCore.currentAvatarProject
                enabled: true
                anchors.verticalCenter: parent.verticalCenter
                anchors.right: parent.right
                text: qsTr("Open Project Folder")
                color: hifi.buttons.blue
                colorScheme: root.colorScheme
                width: 200
                height: 40
                onClicked: {
                    fileDialogHelper.openDirectory(fileDialogHelper.pathToUrl(AvatarPackagerCore.currentAvatarProject.projectFolderPath));
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
        upload(false);
    }
    function uploadUpdate() {
        upload(true);
    }

    Connections {
        target: root.uploader
        onStateChanged: {
            root.hasSuccessfullyUploaded = newState >= 4;
        }
    }

    function upload(updateExisting) {
        root.uploader = AvatarPackagerCore.currentAvatarProject.upload(updateExisting);
        console.log("uploader: "+ root.uploader);
        root.uploader.send();
        avatarPackager.state = AvatarPackagerState.projectUpload;
    }

    function showConfirmUploadPopup() {
        popup.titleText = 'Overwrite Avatar';
        popup.bodyText = 'You have previously uploaded the avatar file from this project.' +
                         ' This will overwrite that avatar and you won’t be able to access the older version.';

        popup.button1text = 'CREATE NEW';
        popup.button2text = 'OVERWRITE';

        popup.onButton2Clicked = function() {
            popup.close();
            uploadUpdate();
        };
        popup.onButton1Clicked = function() {
            popup.close();
            showConfirmCreateNewPopup();
        };

        popup.open();
    }

    function showConfirmCreateNewPopup(confirmCallback) {
        popup.titleText = 'Create New';
        popup.bodyText = 'This will upload your current files with the same avatar name.' +
                         ' You will lose the ability to update the previously uploaded avatar. Are you sure you want to continue?';

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

    HiFiGlyphs {
        id: errorsGlyph
        visible: !AvatarPackagerCore.currentAvatarProject || AvatarPackagerCore.currentAvatarProject.hasErrors
        text: hifi.glyphs.alert
        size: 315
        color: "#EA4C5F"
        anchors {
            top: parent.top
            topMargin: -30
            horizontalCenter: parent.horizontalCenter
        }
    }

    Image {
        id: successGlyph
        visible: AvatarPackagerCore.currentAvatarProject && !AvatarPackagerCore.currentAvatarProject.hasErrors
        anchors {
            top: parent.top
            topMargin: 52
            horizontalCenter: parent.horizontalCenter
        }
        width: 149.6
        height: 149
        source: "../../../icons/checkmark-stroke.svg"
    }

    RalewayRegular {
        id: doctorStatusMessage

        states: [
            State {
               when: AvatarPackagerCore.currentAvatarProject && !AvatarPackagerCore.currentAvatarProject.hasErrors
               name: "noErrors"
                PropertyChanges {
                    target: doctorStatusMessage
                    text: "Your avatar looks fine."
                }
            },
            State {
                when: !AvatarPackagerCore.currentAvatarProject || AvatarPackagerCore.currentAvatarProject.hasErrors
                name: "errors"
                PropertyChanges {
                    target: doctorStatusMessage
                    text: "Your avatar has a few issues."
                }
            }
        ]
        color: 'white'
        size: 20

        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: errorsGlyph.bottom

        wrapMode: Text.Wrap
    }

    RalewayRegular {
        id: notForSaleMessage

        visible: root.hasSuccessfullyUploaded

        color: 'white'
        linkColor: '#00B4EF'
        size: 20

        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: doctorStatusMessage.bottom
        anchors.topMargin: 10

        anchors.bottomMargin: 24

        wrapMode: Text.Wrap
        text: "This item is not for sale yet, <a href='#'>learn more</a>."

        onLinkActivated: {
            Qt.openUrlExternally("https://docs.vircadia.com/sell/add-item/upload-avatar.html");
        }
    }

    RalewayRegular {
        id: showErrorsLink

        color: 'white'
        linkColor: '#00B4EF'

        visible: AvatarPackagerCore.currentAvatarProject && AvatarPackagerCore.currentAvatarProject.hasErrors

        anchors {
            top: notForSaleMessage.bottom
            horizontalCenter: parent.horizontalCenter
        }

        size: 28

        text: "<a href='toggle'>View all errors</a>"

        onLinkActivated: {
            avatarPackager.state = AvatarPackagerState.avatarDoctorErrorReport;
        }
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
        anchors.bottom: showFilesText.top
        anchors.bottomMargin: 24

        wrapMode: Text.Wrap

        // FIXME: Restore original text if ability to upload to hosted location is added.
        //text: "You can upload your files to our servers to always access them, and to make your avatar visible to other users."
        text: "Your files are ready to be uploaded to a server to make your avatar visible to other users."
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

        text: AvatarPackagerCore.currentAvatarProject ? AvatarPackagerCore.currentAvatarProject.projectFiles.length + " files in project. <a href='toggle'>See list</a>" : ""

        onLinkActivated: fileListPopup.open()
    }

    Rectangle {
        id: loginRequiredMessage

        // FIXME: Re-enable if ability to upload to hosted location is added.
        /*
        visible: !Account.loggedIn
        height: !Account.loggedIn ? loginRequiredTextRow.height + 20 : 0
        */
        visible: false
        height: 0

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
