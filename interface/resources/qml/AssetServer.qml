//
//  AssetServer.qml
//
//  Created by Clement on  3/1/16
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import QtQuick 2.5
import QtQuick.Controls 1.4
import QtQuick.Dialogs 1.2 as OriginalDialogs
import Qt.labs.settings 1.0

import "styles-uit"
import "controls-uit" as HifiControls
import "windows-uit"
import "dialogs"

Window {
    id: root
    objectName: "AssetServer"
    title: "My Asset Server"
    resizable: true
    destroyOnInvisible: true
    x: 40; y: 40
    implicitWidth: 384; implicitHeight: 640
    minSize: Qt.vector2d(200, 300)

    property int colorScheme: hifi.colorSchemes.dark

    HifiConstants { id: hifi }

    property var scripts: ScriptDiscoveryService;
    property var assetProxyModel: Assets.proxyModel;
    property var assetMappingsModel: Assets.mappingModel;
    property var currentDirectory;
    property alias currentFileUrl: fileUrlTextField.text;

    Settings {
        category: "Overlay.AssetServer"
        property alias x: root.x
        property alias y: root.y
        property alias directory: root.currentDirectory
    }

    Component.onCompleted: {
        assetMappingsModel.errorGettingMappings.connect(handleGetMappingsError)
        reload()
    }

    function doDeleteFile(path) {
        console.log("Deleting " + path);

        Assets.deleteMappings(path, function(err) {
            if (err) {
                console.log("Error deleting path: ", path, err);

                box = errorMessageBox("There was an error deleting:\n" + path + "\n" + Assets.getErrorString(err));
                box.selected.connect(reload);
            } else {
                console.log("Finished deleting path: ", path);
                reload();
            }
        });

    }
    function doUploadFile(path, mapping, addToWorld) {
        console.log("Uploading " + path + " to " + mapping + " (addToWorld: " + addToWorld + ")");


    }
    function doRenameFile(oldPath, newPath) {
        console.log("Renaming " + oldPath + " to " + newPath);

        Assets.renameMapping(oldPath, newPath, function(err) {
            if (err) {
                console.log("Error renaming: ", oldPath, "=>", newPath, " - error ", err);
                box = errorMessageBox("There was an error renaming:\n" + oldPath + " to " + newPath + "\n" + Assets.getErrorString(err));
                box.selected.connect(reload);
            } else {
                console.log("Finished rename: ", oldPath, "=>", newPath);
            }

            reload();
        });
    }

    function fileExists(path) {
        return Assets.isKnownMapping(path);
    }

    function askForOverride(path, callback) {
        var object = desktop.messageBox({
            icon: hifi.icons.question,
            buttons: OriginalDialogs.StandardButton.Yes | OriginalDialogs.StandardButton.No,
            defaultButton: OriginalDialogs.StandardButton.Yes,
            title: "Overwrite File",
            text: path + "\n" + "This file already exists. Do you want to overwrite it?"
        });
        object.selected.connect(function(button) {
            if (button === OriginalDialogs.StandardButton.Yes) {
                callback();
            }
        });
    }

    function canAddToWorld() {
        var supportedExtensions = [/\.fbx\b/i, /\.obj\b/i];
        var path = assetProxyModel.data(treeView.currentIndex, 0x100);

        return supportedExtensions.reduce(function(total, current) {
            return total | new RegExp(current).test(path);
        }, false);
    }

    function reload() {
        print("reload");
        Assets.mappingModel.refresh();
    }

    function handleGetMappingsError(errorCode) {
        errorMessageBox(
            "There was a problem retreiving the list of assets from your Asset Server.\n"
            + Assets.getErrorString(errorCode)
        );
    }

    function addToWorld() {
        var url = assetProxyModel.data(treeView.currentIndex, 0x103);
        if (!url) {
            return;
        }
        var addPosition = Vec3.sum(MyAvatar.position, Vec3.multiply(2, Quat.getFront(MyAvatar.orientation)));
        Entities.addModelEntity(url, addPosition);
    }

    function copyURLToClipboard() {
        if (!index) {
            index = treeView.currentIndex;
        }

        var url = assetProxyModel.data(treeView.currentIndex, 0x103);
        if (!url) {
            return;
        }
        Window.copyToClipboard(url);
    }

    function renameFile(index) {
        if (!index) {
            index = treeView.currentIndex;
        }
        var path = assetProxyModel.data(index, 0x100);
        if (!path) {
            return;
        }

        var object = desktop.inputDialog({
            label: "Enter new path:",
            current: path,
            placeholderText: "Enter path here"
        });
        object.selected.connect(function(destinationPath) {
            if (path == destinationPath) {
                return;
            }
            if (fileExists(destinationPath)) {
                askForOverride(destinationPath, function() {
                    doRenameFile(path, destinationPath);
                });
            } else {
                doRenameFile(path, destinationPath);
            }
        });
    }
    function deleteFile(index) {
        if (!index) {
            index = treeView.currentIndex;
        }
        var path = assetProxyModel.data(index, 0x100);
        if (!path) {
            return;
        }

        var isFolder = assetProxyModel.data(treeView.currentIndex, 0x101);
        var typeString = isFolder ? 'folder' : 'file';

        var object = desktop.messageBox({
            icon: hifi.icons.question,
            buttons: OriginalDialogs.StandardButton.Yes + OriginalDialogs.StandardButton.No,
            defaultButton: OriginalDialogs.StandardButton.Yes,
            title: "Delete",
            text: "You are about to delete the following " + typeString + ":\n" + path + "\nDo you want to continue?"
        });
        object.selected.connect(function(button) {
            if (button === OriginalDialogs.StandardButton.Yes) {
                doDeleteFile(path);
            }
        });
    }

    function chooseClicked() {
        var browser = desktop.fileDialog({
            selectDirectory: false,
            dir: currentDirectory
        });
        browser.selectedFile.connect(function(url){
            fileUrlTextField.text = fileDialogHelper.urlToPath(url);
            currentDirectory = browser.dir;
        });
    }

    property var uploadOpen: false;
    function uploadClicked() {
        if(uploadOpen) {
            return;
        }
        uploadOpen = true;

        var fileUrl = fileUrlTextField.text
        var addToWorld = addToWorldCheckBox.checked

        var path = assetProxyModel.data(treeView.currentIndex, 0x100);
        var directory = path ? path.slice(0, path.lastIndexOf('/') + 1) : "/";
        var filename = fileUrl.slice(fileUrl.lastIndexOf('/') + 1);

        Assets.uploadFile(fileUrl, directory + filename, function(err) {
            if (err) {
                console.log("Error uploading: ", fileUrl, " - error ", err);
                errorMessage("There was an error uploading:\n" + fileUrl + "\n\nPlease try again.");
            } else {
                console.log("Finished uploading: ", fileUrl);
            }

            reload();
        });
        uploadOpen = false;
    }

    function errorMessageBox(message) {
        return desktop.messageBox({
            icon: hifi.icons.warning,
            defaultButton: OriginalDialogs.StandardButton.Ok,
            title: "Error",
            text: message
        });
    }

    Column {
        width: pane.contentWidth

        HifiControls.ContentSection {
            id: assetDirectory
            name: "Asset Directory"
            spacing: hifi.dimensions.contentSpacing.y
            isFirst: true

            Row {
                id: buttonRow
                anchors.left: parent.left
                anchors.right: parent.right
                spacing: hifi.dimensions.contentSpacing.x

                HifiControls.GlyphButton {
                    glyph: hifi.glyphs.reload
                    color: hifi.buttons.white
                    colorScheme: root.colorScheme
                    height: 26
                    width: 26

                    onClicked: root.reload()
                }

                HifiControls.Button {
                    text: "ADD TO WORLD"
                    color: hifi.buttons.white
                    colorScheme: root.colorScheme
                    height: 26
                    width: 120

                    enabled: canAddToWorld()

                    onClicked: root.addToWorld()
                }

                HifiControls.Button {
                    text: "RENAME"
                    color: hifi.buttons.white
                    colorScheme: root.colorScheme
                    height: 26
                    width: 80

                    onClicked: root.renameFile()
                }

                HifiControls.Button {
                    id: deleteButton

                    text: "DELETE"
                    color: hifi.buttons.red
                    colorScheme: root.colorScheme
                    height: 26
                    width: 80

                    onClicked: root.deleteFile()
                }
            }

            Menu {
                id: contextMenu
                title: "Edit"
                property var url: ""
                property var currentIndex: null

                MenuItem {
                    text: "Copy URL"
                    onTriggered: {
                        copyURLToClipboard(contextMenu.currentIndex);
                    }
                }

                MenuItem {
                    text: "Rename"
                    onTriggered: {
                        renameFile(contextMenu.currentIndex);
                    }
                }

                MenuItem {
                    text: "Delete"
                    onTriggered: {
                        deleteFile(contextMenu.currentIndex);
                    }
                }
            }

            HifiControls.Tree {
                id: treeView
                height: 400
                treeModel: assetProxyModel
                colorScheme: root.colorScheme
                anchors.left: parent.left
                anchors.right: parent.right
                MouseArea {
                    propagateComposedEvents: true
                    anchors.fill: parent
                    acceptedButtons: Qt.RightButton
                    onClicked: {
                        var index = treeView.indexAt(mouse.x, mouse.y);
                        contextMenu.currentIndex = index;
                        contextMenu.popup();
                    }
                }
            }
        }

        HifiControls.ContentSection {
            id: uploadSection
            name: ""
            spacing: hifi.dimensions.contentSpacing.y

            HifiControls.TextField {
                id: fileUrlTextField
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.rightMargin: chooseButton.width + hifi.dimensions.contentSpacing.x

                label: "Upload File"
                placeholderText: "Paste URL or choose file"
                colorScheme: root.colorScheme
            }

            Item {
                // Take the chooseButton out of the column flow.
                id: chooseButtonContainer
                anchors.top: fileUrlTextField.top
                anchors.right: parent.right

                HifiControls.Button {
                    id: chooseButton
                    anchors.right: parent.right

                    text: "Choose"
                    color: hifi.buttons.white
                    colorScheme: root.colorScheme
                    enabled: true

                    width: 100

                    onClicked: root.chooseClicked()
                }
            }

            HifiControls.CheckBox {
                id: addToWorldCheckBox
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.rightMargin: uploadButton.width + hifi.dimensions.contentSpacing.x

                text: "Add to world on upload"
                checked: false
            }

            Item {
                // Take the uploadButton out of the column flow.
                id: uploadButtonContainer
                anchors.top: addToWorldCheckBox.top
                anchors.right: parent.right

                HifiControls.Button {
                    id: uploadButton
                    anchors.right: parent.right

                    text: "Upload"
                    color: hifi.buttons.blue
                    colorScheme: root.colorScheme
                    height: 30
                    width: 155

                    enabled: fileUrlTextField.text != ""
                    onClicked: root.uploadClicked()
                }
            }

            HifiControls.VerticalSpacer {}
        }
    }
}
