//
//  TabletAssetServer.qml
//
//  Created by Vlad Stelmahovsky on  3/3/17
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import QtQuick 2.5
import QtQuick.Controls 1.4
import QtQuick.Dialogs 1.2 as OriginalDialogs
import Qt.labs.settings 1.0

import "../../styles-uit"
import "../../controls-uit" as HifiControls
import "../../windows"

Rectangle {
    id: root
    objectName: "AssetServer"

    property string title: "Asset Browser"
    property bool keyboardRaised: false

    signal sendToScript(var message);
    property bool isHMD: false

    color: hifi.colors.baseGray

    property int colorScheme: hifi.colorSchemes.dark

    HifiConstants { id: hifi }

    property var scripts: ScriptDiscoveryService;
    property var assetProxyModel: Assets.proxyModel;
    property var assetMappingsModel: Assets.mappingModel;
    property var currentDirectory;
    property var selectedItems: treeView.selection.selectedIndexes.length;

    Settings {
        category: "Overlay.AssetServer"
        property alias directory: root.currentDirectory
    }

    Component.onCompleted: {
        isHMD = HMD.active;
        ApplicationInterface.uploadRequest.connect(uploadClicked);
        assetMappingsModel.errorGettingMappings.connect(handleGetMappingsError);
        reload();
    }

    function doDeleteFile(path) {
        console.log("Deleting " + path);

        Assets.deleteMappings(path, function(err) {
            if (err) {
                console.log("Asset browser - error deleting path: ", path, err);

                box = errorMessageBox("There was an error deleting:\n" + path + "\n" + err);
                box.selected.connect(reload);
            } else {
                console.log("Asset browser - finished deleting path: ", path);
                reload();
            }
        });

    }

    function doRenameFile(oldPath, newPath) {

        if (newPath[0] !== "/") {
            newPath = "/" + newPath;
        }

        if (oldPath[oldPath.length - 1] === '/' && newPath[newPath.length - 1] != '/') {
            // this is a folder rename but the user neglected to add a trailing slash when providing a new path
            newPath = newPath + "/";
        }

        if (Assets.isKnownFolder(newPath)) {
            box = errorMessageBox("Cannot overwrite existing directory.");
            box.selected.connect(reload);
        }

        console.log("Asset browser - renaming " + oldPath + " to " + newPath);

        Assets.renameMapping(oldPath, newPath, function(err) {
            if (err) {
                console.log("Asset browser - error renaming: ", oldPath, "=>", newPath, " - error ", err);
                box = errorMessageBox("There was an error renaming:\n" + oldPath + " to " + newPath + "\n" + err);
                box.selected.connect(reload);
            } else {
                console.log("Asset browser - finished rename: ", oldPath, "=>", newPath);
            }

            reload();
        });
    }

    function fileExists(path) {
        return Assets.isKnownMapping(path);
    }

    function askForOverwrite(path, callback) {
        var object = tabletRoot.messageBox({
                                            icon: hifi.icons.question,
                                            buttons: OriginalDialogs.StandardButton.Yes | OriginalDialogs.StandardButton.No,
                                            defaultButton: OriginalDialogs.StandardButton.No,
                                            title: "Overwrite File",
                                            text: path + "\n" + "This file already exists. Do you want to overwrite it?"
                                        });
        object.selected.connect(function(button) {
            if (button === OriginalDialogs.StandardButton.Yes) {
                callback();
            }
        });
    }

    function canAddToWorld(path) {
        var supportedExtensions = [/\.fbx\b/i, /\.obj\b/i];
        
        if (selectedItems > 1) {
            return false;
        }

        return supportedExtensions.reduce(function(total, current) {
            return total | new RegExp(current).test(path);
        }, false);
    }
    
    function canRename() {    
        if (treeView.selection.hasSelection && selectedItems == 1) {
            return true;
        } else {
            return false;
        }
    }

    function clear() {
        Assets.mappingModel.clear();
    }

    function reload() {
        Assets.mappingModel.refresh();
        treeView.selection.clear();
    }

    function handleGetMappingsError(errorString) {
        errorMessageBox(
                    "There was a problem retreiving the list of assets from your Asset Server.\n"
                    + errorString
                    );
    }

    function addToWorld() {
        var defaultURL = assetProxyModel.data(treeView.selection.currentIndex, 0x103);

        if (!defaultURL || !canAddToWorld(defaultURL)) {
            return;
        }

        var SHAPE_TYPE_NONE = 0;
        var SHAPE_TYPE_SIMPLE_HULL = 1;
        var SHAPE_TYPE_SIMPLE_COMPOUND = 2;
        var SHAPE_TYPE_STATIC_MESH = 3;
        var SHAPE_TYPE_BOX = 4;
        var SHAPE_TYPE_SPHERE = 5;
        
        var SHAPE_TYPES = [];
        SHAPE_TYPES[SHAPE_TYPE_NONE] = "No Collision";
        SHAPE_TYPES[SHAPE_TYPE_SIMPLE_HULL] = "Basic - Whole model";
        SHAPE_TYPES[SHAPE_TYPE_SIMPLE_COMPOUND] = "Good - Sub-meshes";
        SHAPE_TYPES[SHAPE_TYPE_STATIC_MESH] = "Exact - All polygons";
        SHAPE_TYPES[SHAPE_TYPE_BOX] = "Box";
        SHAPE_TYPES[SHAPE_TYPE_SPHERE] = "Sphere";
        
        var SHAPE_TYPE_DEFAULT = SHAPE_TYPE_STATIC_MESH;
        var DYNAMIC_DEFAULT = false;
        var prompt = tabletRoot.customInputDialog({
                                                   textInput: {
                                                       label: "Model URL",
                                                       text: defaultURL
                                                   },
                                                   comboBox: {
                                                       label: "Automatic Collisions",
                                                       index: SHAPE_TYPE_DEFAULT,
                                                       items: SHAPE_TYPES
                                                   },
                                                   checkBox: {
                                                       label: "Dynamic",
                                                       checked: DYNAMIC_DEFAULT,
                                                       disableForItems: [
                                                           SHAPE_TYPE_STATIC_MESH
                                                       ],
                                                       checkStateOnDisable: false,
                                                       warningOnDisable: "Models with 'Exact' automatic collisions cannot be dynamic, and should not be used as floors"
                                                   }
                                               });

        prompt.selected.connect(function (jsonResult) {
            if (jsonResult) {
                var result = JSON.parse(jsonResult);
                var url = result.textInput.trim();
                var shapeType;
                switch (result.comboBox) {
                case SHAPE_TYPE_SIMPLE_HULL:
                    shapeType = "simple-hull";
                    break;
                case SHAPE_TYPE_SIMPLE_COMPOUND:
                    shapeType = "simple-compound";
                    break;
                case SHAPE_TYPE_STATIC_MESH:
                    shapeType = "static-mesh";
                    break;
                case SHAPE_TYPE_BOX:
                    shapeType = "box";
                    break;
                case SHAPE_TYPE_SPHERE:
                    shapeType = "sphere";
                    break;
                default:
                    shapeType = "none";
                }

                var dynamic = result.checkBox !== null ? result.checkBox : DYNAMIC_DEFAULT;
                if (shapeType === "static-mesh" && dynamic) {
                    // The prompt should prevent this case
                    print("Error: model cannot be both static mesh and dynamic.  This should never happen.");
                } else if (url) {
                    var name = assetProxyModel.data(treeView.selection.currentIndex);
                    var addPosition = Vec3.sum(MyAvatar.position, Vec3.multiply(2, Quat.getFront(MyAvatar.orientation)));
                    var gravity;
                    if (dynamic) {
                        // Create a vector <0, -10, 0>.  { x: 0, y: -10, z: 0 } won't work because Qt is dumb and this is a
                        // different scripting engine from QTScript.
                        gravity = Vec3.multiply(Vec3.fromPolar(Math.PI / 2, 0), 10);
                    } else {
                        gravity = Vec3.multiply(Vec3.fromPolar(Math.PI / 2, 0), 0);
                    }

                    print("Asset browser - adding asset " + url + " (" + name + ") to world.");

                    // Entities.addEntity doesn't work from QML, so we use this.
                    Entities.addModelEntity(name, url, shapeType, dynamic, addPosition, gravity);
                }
            }
        });
    }

    function copyURLToClipboard(index) {
        if (!index) {
            index = treeView.selection.currentIndex;
        }

        var url = assetProxyModel.data(treeView.selection.currentIndex, 0x103);
        if (!url) {
            return;
        }
        Window.copyToClipboard(url);
    }

    function renameEl(index, data) {
        if (!index) {
            return false;
        }

        var path = assetProxyModel.data(index, 0x100);
        if (!path) {
            return false;
        }

        var destinationPath = path.split('/');
        destinationPath[destinationPath.length - (path[path.length - 1] === '/' ? 2 : 1)] = data;
        destinationPath = destinationPath.join('/').trim();

        if (path === destinationPath) {
            return;
        }
        if (!fileExists(destinationPath)) {
            doRenameFile(path, destinationPath);
        }
    }
    function renameFile(index) {
        if (!index) {
            index = treeView.selection.currentIndex;
        }

        var path = assetProxyModel.data(index, 0x100);
        if (!path) {
            return;
        }

        var object = tabletRoot.inputDialog({
                                             label: "Enter new path:",
                                             current: path,
                                             placeholderText: "Enter path here"
                                         });
        object.selected.connect(function(destinationPath) {
            destinationPath = destinationPath.trim();

            if (path === destinationPath) {
                return;
            }
            if (fileExists(destinationPath)) {
                askForOverwrite(destinationPath, function() {
                    doRenameFile(path, destinationPath);
                });
            } else {
                doRenameFile(path, destinationPath);
            }
        });
    }
    function deleteFile(index) {
        var path = [];
        
        if (!index) {
            for (var i = 0; i < selectedItems; i++) {
                 treeView.selection.setCurrentIndex(treeView.selection.selectedIndexes[i], 0x100);
                 index = treeView.selection.currentIndex;
                 path[i] = assetProxyModel.data(index, 0x100);                  
            }
        }
        
        if (!path) {
            return;
        }

        var modalMessage = "";
        var items = selectedItems.toString();
        var isFolder = assetProxyModel.data(treeView.selection.currentIndex, 0x101);
        var typeString = isFolder ? 'folder' : 'file';
        
        if (selectedItems > 1) {
            modalMessage = "You are about to delete " + items + " items \nDo you want to continue?";
        } else {
            modalMessage = "You are about to delete the following " + typeString + ":\n" + path + "\nDo you want to continue?";
        }

        var object = tabletRoot.messageBox({
                                            icon: hifi.icons.question,
                                            buttons: OriginalDialogs.StandardButton.Yes + OriginalDialogs.StandardButton.No,
                                            defaultButton: OriginalDialogs.StandardButton.Yes,
                                            title: "Delete",
                                            text: modalMessage
                                        });
        object.selected.connect(function(button) {
            if (button === OriginalDialogs.StandardButton.Yes) {
                doDeleteFile(path);
            }
        });
    }

    Timer {
        id: doUploadTimer
        property var url
        property bool isConnected: false
        interval: 5
        repeat: false
        running: false
    }

    property bool uploadOpen: false;
    Timer {
        id: timer
    }
    function uploadClicked(fileUrl) {
        if (uploadOpen) {
            return;
        }
        uploadOpen = true;

        function doUpload(url, dropping) {
            var fileUrl = fileDialogHelper.urlToPath(url);

            var path = assetProxyModel.data(treeView.selection.currentIndex, 0x100);
            var directory = path ? path.slice(0, path.lastIndexOf('/') + 1) : "/";
            var filename = fileUrl.slice(fileUrl.lastIndexOf('/') + 1);

            Assets.uploadFile(fileUrl, directory + filename,
                              function() {
                                  // Upload started
                                  uploadSpinner.visible = true;
                                  uploadButton.enabled = false;
                                  uploadProgressLabel.text = "In progress...";
                              },
                              function(err, path) {
                                  print(err, path);
                                  if (err === "") {
                                      uploadProgressLabel.text = "Upload Complete";
                                      timer.interval = 1000;
                                      timer.repeat = false;
                                      timer.triggered.connect(function() {
                                          uploadSpinner.visible = false;
                                          uploadButton.enabled = true;
                                          uploadOpen = false;
                                      });
                                      timer.start();
                                      console.log("Asset Browser - finished uploading: ", fileUrl);
                                      reload();
                                  } else {
                                      uploadSpinner.visible = false;
                                      uploadButton.enabled = true;
                                      uploadOpen = false;

                                      if (err !== -1) {
                                          console.log("Asset Browser - error uploading: ", fileUrl, " - error ", err);
                                          var box = errorMessageBox("There was an error uploading:\n" + fileUrl + "\n" + err);
                                          box.selected.connect(reload);
                                      }
                                  }
                              }, dropping);
        }

        function initiateUpload(url) {
            doUpload(doUploadTimer.url, false);
        }

        if (fileUrl) {
            doUpload(fileUrl, true);
        } else {
            var browser = tabletRoot.fileDialog({
                                                 selectDirectory: false,
                                                 dir: currentDirectory
                                             });

            browser.canceled.connect(function() {
                uploadOpen = false;
            });

            browser.selectedFile.connect(function(url) {
                currentDirectory = browser.dir;

                // Initiate upload from a timer so that file browser dialog can close beforehand.
                doUploadTimer.url = url;
                if (!doUploadTimer.isConnected) {
                    doUploadTimer.triggered.connect(function() { initiateUpload(); });
                    doUploadTimer.isConnected = true;
                }
                doUploadTimer.start();
            });
        }
    }

    function errorMessageBox(message) {
        return tabletRoot.messageBox({
                                      icon: hifi.icons.warning,
                                      defaultButton: OriginalDialogs.StandardButton.Ok,
                                      title: "Error",
                                      text: message
                                  });
    }

    Column {
        width: parent.width
        spacing: 10

        HifiControls.TabletContentSection {
            id: assetDirectory
            name: "Asset Directory"
            isFirst: true

            HifiControls.VerticalSpacer {}

            Row {
                id: buttonRow
                width: parent.width
                height: 30
                spacing: hifi.dimensions.contentSpacing.x

                HifiControls.GlyphButton {
                    glyph: hifi.glyphs.reload
                    color: hifi.buttons.black
                    colorScheme: root.colorScheme
                    width: hifi.dimensions.controlLineHeight

                    onClicked: root.reload()
                }

                HifiControls.Button {
                    text: "Add To World"
                    color: hifi.buttons.black
                    colorScheme: root.colorScheme
                    width: 120

                    enabled: canAddToWorld(assetProxyModel.data(treeView.selection.currentIndex, 0x100))

                    onClicked: root.addToWorld()
                }

                HifiControls.Button {
                    text: "Rename"
                    color: hifi.buttons.black
                    colorScheme: root.colorScheme
                    width: 80

                    onClicked: root.renameFile()
                    enabled: canRename()
                }

                HifiControls.Button {
                    id: deleteButton

                    text: "Delete"
                    color: hifi.buttons.red
                    colorScheme: root.colorScheme
                    width: 80

                    onClicked: root.deleteFile()
                    enabled: treeView.selection.hasSelection
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

        }
        HifiControls.Tree {
            id: treeView
            height: 290
            anchors.leftMargin: hifi.dimensions.contentMargin.x + 2  // Extra for border
            anchors.rightMargin: hifi.dimensions.contentMargin.x + 2  // Extra for border
            anchors.left: parent.left
            anchors.right: parent.right

            treeModel: assetProxyModel
            canEdit: true
            colorScheme: root.colorScheme
            selectionMode: SelectionMode.ExtendedSelection

            modifyEl: renameEl

            MouseArea {
                propagateComposedEvents: true
                anchors.fill: parent
                acceptedButtons: Qt.RightButton
                onClicked: {
                    if (!HMD.active) {  // Popup only displays properly on desktop
                        var index = treeView.indexAt(mouse.x, mouse.y);
                        treeView.selection.setCurrentIndex(index, 0x0002);
                        contextMenu.currentIndex = index;
                        contextMenu.popup();
                    }
                }
            }
        }


        HifiControls.TabletContentSection {
            id: uploadSection
            name: "Upload A File"
            spacing: hifi.dimensions.contentSpacing.y
            //anchors.bottom: parent.bottom
            height: 65
            anchors.left: parent.left
            anchors.right: parent.right

            Item {
                height: parent.height
                width: parent.width
                HifiControls.Button {
                    id: uploadButton
                    anchors.right: parent.right

                    text: "Choose File"
                    color: hifi.buttons.blue
                    colorScheme: root.colorScheme
                    height: 30
                    width: 155

                    onClicked: uploadClickedTimer.running = true

                    // For some reason trigginer an API that enters
                    // an internal event loop directly from the button clicked
                    // trigger below causes the appliction to behave oddly.
                    // Most likely because the button onClicked handling is never
                    // completed until the function returns.
                    // FIXME find a better way of handling the input dialogs that
                    // doesn't trigger this.
                    Timer {
                        id: uploadClickedTimer
                        interval: 5
                        repeat: false
                        running: false
                        onTriggered: uploadClicked();
                    }
                }

                Item {
                    id: uploadSpinner
                    visible: false
                    anchors.top: parent.top
                    anchors.left: parent.left
                    width: 40
                    height: 32

                    Image {
                        id: image
                        width: 24
                        height: 24
                        source: "../../../images/Loading-Outer-Ring.png"
                        RotationAnimation on rotation {
                            loops: Animation.Infinite
                            from: 0
                            to: 360
                            duration: 2000
                        }
                    }
                    Image {
                        width: 24
                        height: 24
                        source: "../../../images/Loading-Inner-H.png"
                    }
                    HifiControls.Label {
                        id: uploadProgressLabel
                        anchors.left: image.right
                        anchors.leftMargin: 10
                        anchors.verticalCenter: image.verticalCenter
                        text: "In progress..."
                        colorScheme: root.colorScheme
                    }
                }
            }
        }
    }
}

