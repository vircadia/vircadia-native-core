//
//  AssetsManager.qml
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

Window {
    id: root
    objectName: "AssetsManager"
    title: "Assets Server"
    resizable: true
    destroyOnInvisible: true
    x: 40; y: 40
    implicitWidth: 384; implicitHeight: 640
    minSize: Qt.vector2d(200, 300)

   colorScheme: hifi.colorSchemes.light

    HifiConstants { id: hifi }

    property var scripts: ScriptDiscoveryService;
    property var scriptsModel: scripts.scriptsModelFilter
    property var runningScriptsModel: ListModel { }

    Settings {
        category: "Overlay.AssetManager"
        property alias x: root.x
        property alias y: root.y
    }

    Component.onCompleted: updateRunningScripts()

    function updateRunningScripts() {
        var runningScripts = ScriptDiscoveryService.getRunning();
        runningScriptsModel.clear()
        for (var i = 0; i < runningScripts.length; ++i) {
            runningScriptsModel.append(runningScripts[i]);
        }
    }

    Column {
        width: pane.contentWidth

        HifiControls.ContentSection {
            name: "Asset Directory"
            spacing: hifi.dimensions.contentSpacing.y
            colorScheme: root.colorScheme
            isFirst: true

            Row {
                id: buttonRow
                anchors.left: parent.left
                anchors.right: parent.right

                HifiControls.Button {
                    text: "<"
                    color: hifi.buttons.white
                    colorScheme: root.colorScheme
                    height: 26
                    width: 26
                }

                HifiControls.Button {
                    text: "O"
                    color: hifi.buttons.white
                    colorScheme: root.colorScheme
                    height: 26
                    width: 26
                }
            }

            Item {
                // Take the browseButton out of the column flow.
                id: deleteButtonContainer
                anchors.top: buttonRow.top
                anchors.right: parent.right

                HifiControls.Button {
                    id: deleteButton
                    anchors.right: parent.right

                    text: "DELETE SELECTION"
                    color: hifi.buttons.red
                    colorScheme: root.colorScheme
                    height: 26
                    width: 130
                }
            }

            HifiControls.Tree {
                id: treeView
                height: 155
                treeModel: scriptsModel
                colorScheme: root.colorScheme
                anchors.left: parent.left
                anchors.right: parent.right
            }
        }

        HifiControls.ContentSection {
            name: ""
            spacing: hifi.dimensions.contentSpacing.y
            colorScheme: root.colorScheme

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
                // Take the browseButton out of the column flow.
                id: uploadButtonContainer
                anchors.top: addToScene.top
                anchors.right: parent.right

                HifiControls.Button {
                    id: uploadButton
                    anchors.right: parent.right

                    text: "Upload"
                    color: hifi.buttons.blue
                    colorScheme: root.colorScheme
                    enabled: true
                    height: 30
                    width: 155
                }
            }
        }
    }
}

