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

    visible: false
    anchors.fill: parent
    anchors.margins: 10

    property var footer: Item {
        id: uploadFooter
        anchors.fill: parent
        anchors.rightMargin: 17
        HifiControls.Button {
            id: uploadButton
            enabled: Account.loggedIn
            //width: parent.width
            //anchors.bottom: parent.bottom
            anchors.verticalCenter: parent.verticalCenter
            anchors.right: parent.right
            text: qsTr("Upload")
            color: hifi.buttons.blue
            colorScheme: root.colorScheme
            width: 133
            height: 40
            onClicked: function() {
                if (AvatarPackagerCore.currentAvatarProject.fst.hasMarketplaceID()) {
                    showConfirmUploadPopup(uploadNew, uploadUpdate);
                } else {
                    uploadNew();
                }
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
            try {
                var response = JSON.parse(root.uploader.responseData);
                console.log("Uploader complete! " + response);
                uploadStatus.text = response.status;
            } catch (e) {
                console.log("Error parsing JSON: " + root.uploader.reponseData);
            }
        });
        root.uploader.send();
        avatarPackager.state = "project-upload";
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

        color: 'white'
        size: 20

        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top

        anchors.bottomMargin: 24

        wrapMode: Text.Wrap

        text: "Click \"Update\" to overwrite the hosted files and update the avatar in your inventory. You will have to “Wear” the avatar again to see changes."
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

    Rectangle {
        id: fileList

        visible: false

        color: "#6A6A6A"

        anchors.top: openFolderButton.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: showFilesText.top
        //anchors.bottom: uploadButton.top
        anchors.topMargin: 10
        anchors.bottomMargin: 10
        height: 1000

        ListView {
            anchors.fill: parent
            model: AvatarPackagerCore.currentAvatarProject === null ? [] : AvatarPackagerCore.currentAvatarProject.projectFiles
            delegate: Rectangle {
                width: parent.width
                height: fileText.implicitHeight + 8
                color: (index % 2 == 0) ? "#6A6A6A" : "grey"
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

    RalewayRegular {
        id: showFilesText

        color: 'white'
        linkColor: style.colors.blueHighlight

        visible: AvatarPackagerCore.currentAvatarProject !== null

        anchors.bottom: loginRequiredMessage.top
        anchors.bottomMargin: 10

        size: 20

        text: AvatarPackagerCore.currentAvatarProject.projectFiles.length + " files in the project. <a href='toggle'>" + (fileList.visible ? "Hide" : "Show") + " list</a>"

        onLinkActivated: fileList.visible = !fileList.visible
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
