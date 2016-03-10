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
    title: "Asset Server"
    resizable: true
    destroyOnInvisible: true
    x: 40; y: 40
    implicitWidth: 384; implicitHeight: 640
    minSize: Qt.vector2d(200, 300)

    property int colorScheme: hifi.colorSchemes.dark

    HifiConstants { id: hifi }

    property var scripts: ScriptDiscoveryService;
    property var assetMappingsModel: Assets.proxyModel;
    property var currentDirectory;
    property alias currentFileUrl: fileUrlTextField.text;

    Settings {
        category: "Overlay.AssetServer"
        property alias x: root.x
        property alias y: root.y
        property alias directory: root.currentDirectory
    }

    function doDeleteFile(path) {
        console.log("Deleting " + path);

        Assets.deleteMappings(path, function(err) {
            if (err) {
                console.log("Error deleting path: ", path, err);
                errorMessage("There was an error deleting:\n" + path + "\n\nPlease try again.");
            } else {
                console.log("Finished deleting path: ", path);
            }

            reload();
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
                errorMessage("There was an error renaming:\n" + oldPath + " to " + newPath + "\n\nPlease try again.");
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
            icon: OriginalDialogs.StandardIcon.Question,
            buttons: OriginalDialogs.StandardButton.Yes | OriginalDialogs.StandardButton.No,
            defaultButton: OriginalDialogs.StandardButton.No,
            text: "Override?",
            informativeText: "The following file already exists:\n" + path +
                             "\nDo you want to override it?"
        });
        object.selected.connect(function(button) {
            if (button === OriginalDialogs.StandardButton.Yes) {
                callback();
            }
        });
    }

    function canAddToWorld() {
        var supportedExtensions = [/\.fbx\b/i, /\.obj\b/i];
        var path = assetMappingsModel.data(treeView.currentIndex, 0x100);

        return supportedExtensions.reduce(function(total, current) {
            return total | new RegExp(current).test(path);
        }, false);
    }

    function reload() {
        print("reload");
        Assets.mappingModel.refresh();
    }
    function addToWorld() {
        var path = assetMappingsModel.data(treeView.currentIndex, 0x100);
        if (!path) {
            return;
        }
    }

    function copyURLToClipboard() {
        var path = assetMappingsModel.data(treeView.currentIndex, 0x103);
        if (!path) {
            return;
        }
        Window.copyToClipboard(path);
    }

    function renameFile() {
        var path = assetMappingsModel.data(treeView.currentIndex, 0x100);
        if (!path) {
            return;
        }

        var object = desktop.inputDialog({
            label: "Enter new path:",
            prefilledText: path,
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
    function deleteFile() {
        var path = assetMappingsModel.data(treeView.currentIndex, 0x100);
        if (!path) {
            return;
        }

        var isFolder = assetMappingsModel.data(treeView.currentIndex, 0x101);
        var typeString = isFolder ? 'folder' : 'file';

        var object = desktop.messageBox({
            icon: OriginalDialogs.StandardIcon.Question,
            buttons: OriginalDialogs.StandardButton.Yes | OriginalDialogs.StandardButton.No,
            defaultButton: OriginalDialogs.StandardButton.No,
            text: "Deleting",
            informativeText: "You are about to delete the following " + typeString + ":\n" +
                             path +
                             "\nDo you want to continue?"
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

    function uploadClicked() {
        var fileUrl = fileUrlTextField.text
        var addToWorld = addToWorldCheckBox.checked

        var path = assetMappingsModel.data(treeView.currentIndex, 0x100);
        var directory = path ? path.slice(0, path.lastIndexOf('/') + 1) : "";
        var filename = fileUrl.slice(fileUrl.lastIndexOf('/') + 1);

        var object = desktop.inputDialog({
            label: "Enter asset path:",
            prefilledText: directory + filename,
            placeholderText: "Enter path here"
        });
        object.selected.connect(function(destinationPath) {
            if (fileExists(destinationPath)) {
                askForOverride(fileUrl, function() {
                    doUploadFile(fileUrl, destinationPath, addToWorld);
                });
            } else {
                doUploadFile(fileUrl, destinationPath, addToWorld);
            }
        });
    }

    function errorMessage(message) {
        desktop.messageBox({
            icon: OriginalDialogs.StandardIcon.Error,
            buttons: OriginalDialogs.StandardButton.Ok,
            text: "Error",
            informativeText: message
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

                HifiControls.GlyphButton {
                    glyph: hifi.glyphs.reload
                    color: hifi.buttons.white
                    colorScheme: root.colorScheme
                    height: 26
                    width: 26

                    onClicked: root.copyURLToClipboard()
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

            HifiControls.Tree {
                id: treeView
                height: 400
                treeModel: assetMappingsModel
                colorScheme: root.colorScheme
                anchors.left: parent.left
                anchors.right: parent.right
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
