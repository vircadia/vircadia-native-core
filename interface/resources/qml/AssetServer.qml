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
    property var scriptsModel: scripts.scriptsModelFilter
    property var currentDirectory: ""

    Settings {
        category: "Overlay.AssetServer"
        property alias x: root.x
        property alias y: root.y
        property alias directory: root.currentDirectory
    }

    function reload() {
        print("reload");
    }
    function addToWorld() {
        print("addToWorld");
    }
    function renameFile() {
        print("renameFile");
    }
    function deleteFile() {
        print("deleteFile");

    }

    function uploadFile(fileUrl, addToScene) {
        var object = desktop.inputDialog({
            label: "Enter asset path",
            //placeholderText: "atp:/myFolder/myFile.ext"
        });
        object.selected.connect(function(value) {
            print(value);
        });
        print("uploadFile: " + fileUrl + " " + addToScene);
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
                treeModel: scriptsModel
                colorScheme: root.colorScheme
                anchors.left: parent.left
                anchors.right: parent.right
            }
        }

        HifiControls.ContentSection {
            id: uploadSection
            name: ""
            spacing: hifi.dimensions.contentSpacing.y

            Component {
                id: fileBrowserBuilder;
                FileDialog { selectDirectory: true }
            }

            HifiControls.TextField {
                id: fileUrl
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
                anchors.top: fileUrl.top
                anchors.right: parent.right

                HifiControls.Button {
                    id: chooseButton
                    anchors.right: parent.right

                    text: "Choose"
                    color: hifi.buttons.white
                    colorScheme: root.colorScheme
                    enabled: true

                    width: 100

                    onClicked: {
                        var browser = fileBrowserBuilder.createObject(desktop, {
                            selectDirectory: false,
                            folder: fileDialogHelper.pathToUrl(currentDirectory)
                        });
                        browser.selectedFile.connect(function(url){
                            console.log(url);
                            fileUrl.text = fileDialogHelper.urlToPath(url);
                        });
                    }
                }
            }

            HifiControls.CheckBox {
                id: addToScene
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.rightMargin: uploadButton.width + hifi.dimensions.contentSpacing.x

                text: "Add to scene on upload"
                checked: false
            }

            Item {
                // Take the uploadButton out of the column flow.
                id: uploadButtonContainer
                anchors.top: addToScene.top
                anchors.right: parent.right

                HifiControls.Button {
                    id: uploadButton
                    anchors.right: parent.right

                    text: "Upload"
                    color: hifi.buttons.blue
                    colorScheme: root.colorScheme
                    height: 30
                    width: 155

                    enabled: fileUrl.text != ""
                    onClicked: root.uploadFile(fileUrl.text, addToScene.checked)
                }
            }
        }
    }
}
