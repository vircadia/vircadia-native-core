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
import QtQuick.Controls.Styles 1.4
import QtQuick.Dialogs 1.2 as OriginalDialogs
import Qt.labs.settings 1.0

import stylesUit 1.0
import controlsUit 1.0 as HifiControls
import "../windows" as Windows
import "../dialogs"

Windows.ScrollingWindow {
    id: root
    objectName: "AssetServer"
    title: "Asset Browser"
    resizable: true
    opacity: parent.opacity
    destroyOnHidden: true
    implicitWidth: 384; implicitHeight: 640
    minSize: Qt.vector2d(200, 300)

    property int colorScheme: hifi.colorSchemes.dark
    property int selectionMode: SelectionMode.ExtendedSelection

    HifiConstants { id: hifi }

    property var scripts: ScriptDiscoveryService;
    property var assetProxyModel: Assets.proxyModel;
    property var assetMappingsModel: Assets.mappingModel;
    property var currentDirectory;
    property var selectedItemCount: treeView.selection.selectedIndexes.length;
    property int updatesCount: 0; // this is used for notifying model-dependent bindings about model updates

    Settings {
        category: "Overlay.AssetServer"
        property alias x: root.x
        property alias y: root.y
        property alias directory: root.currentDirectory
    }

    Component.onCompleted: {
        ApplicationInterface.uploadRequest.connect(uploadClicked);
        assetMappingsModel.errorGettingMappings.connect(handleGetMappingsError);
        assetMappingsModel.autoRefreshEnabled = true;
        assetMappingsModel.updated.connect(function() {
            ++updatesCount;
        });

        reload();
    }

    Component.onDestruction: {
        assetMappingsModel.autoRefreshEnabled = false;
    }

    function letterbox(headerGlyph, headerText, message) {
        letterboxMessage.headerGlyph = headerGlyph;
        letterboxMessage.headerText = headerText;
        letterboxMessage.text = message;
        letterboxMessage.visible = true;
        letterboxMessage.popupRadius = 0;
    }

    function errorMessageBox(message) {
        return desktop.messageBox({
            icon: hifi.icons.warning,
            defaultButton: OriginalDialogs.StandardButton.Ok,
            title: "Error",
            text: message
        });
    }

    function doDeleteFile(paths) {
        console.log("Deleting " + paths);

        Assets.deleteMappings(paths, function(err) {
            if (err) {
                console.log("Asset browser - error deleting paths: ", paths, err);

                box = errorMessageBox("There was an error deleting:\n" + paths + "\n" + err);
                box.selected.connect(reload);
            } else {
                console.log("Asset browser - finished deleting paths: ", paths);
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
        var object = desktop.messageBox({
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
        var supportedExtensions = [/\.fbx\b/i, /\.obj\b/i, /\.jpg\b/i, /\.png\b/i, /\.gltf\b/i, /\.glb\b/i];

        if (selectedItemCount > 1) {
            return false;
        }

        return supportedExtensions.reduce(function(total, current) {
            return total | new RegExp(current).test(path);
        }, false);
    }

    function canRename() {
        if (treeView.selection.hasSelection && selectedItemCount == 1) {
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
    }

    function handleGetMappingsError(errorString) {
        errorMessageBox("There was a problem retrieving the list of assets from your Asset Server.\n" + errorString);
    }

    function addToWorld() {
        var defaultURL = assetProxyModel.data(treeView.selection.currentIndex, 0x103);

        if (!defaultURL || !canAddToWorld(defaultURL)) {
            return;
        }

        var grabbable = MenuInterface.isOptionChecked("Create Entities As Grabbable (except Zones, Particles, and Lights)");

        if (defaultURL.endsWith(".jpg") || defaultURL.endsWith(".png")) {
            Entities.addEntity({
                type: "Image",
                name: assetProxyModel.data(treeView.selection.currentIndex),
                imageURL: defaultURL,
                keepAspectRatio: false,
                dynamic: false,
                collisionless: true,
                grabbable: grabbable,
                position: Vec3.sum(MyAvatar.position, Vec3.multiply(2, Quat.getForward(MyAvatar.orientation))),
                gravity: Vec3.multiply(Vec3.fromPolar(Math.PI / 2, 0), 0)
            });
        } else {
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

            var SHAPE_TYPE_DEFAULT = SHAPE_TYPE_SIMPLE_COMPOUND;
            var DYNAMIC_DEFAULT = false;
            var prompt = desktop.customInputDialog({
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
                    var collisionless = false;
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
                            collisionless = true;
                    }

                    var dynamic = result.checkBox !== null ? result.checkBox : DYNAMIC_DEFAULT;
                    if (shapeType === "static-mesh" && dynamic) {
                        // The prompt should prevent this case
                        print("Error: model cannot be both static mesh and dynamic.  This should never happen.");
                    } else if (url) {
                        var name = assetProxyModel.data(treeView.selection.currentIndex);
                        var addPosition = Vec3.sum(MyAvatar.position, Vec3.multiply(2, Quat.getForward(MyAvatar.orientation)));
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
                        Entities.addModelEntity(name, url, "", shapeType, dynamic, collisionless, grabbable, addPosition, gravity);
                    }
                }
            });
        }
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

        var object = desktop.inputDialog({
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
        var paths = [];

        if (!index) {
            for (var i = 0; i < selectedItemCount; ++i) {
                 index = treeView.selection.selectedIndexes[i];
                 paths[i] = assetProxyModel.data(index, 0x100);
            }
        }

        if (!paths) {
            return;
        }

        var modalMessage = "";
        var items = selectedItemCount.toString();
        var isFolder = assetProxyModel.data(treeView.selection.currentIndex, 0x101);
        var typeString = isFolder ? 'folder' : 'file';

        if (selectedItemCount > 1) {
            modalMessage = "You are about to delete " + items + " items \nDo you want to continue?";
        } else {
            modalMessage = "You are about to delete the following " + typeString + ":\n" + paths + "\nDo you want to continue?";
        }

        var object = desktop.messageBox({
            icon: hifi.icons.question,
            buttons: OriginalDialogs.StandardButton.Yes + OriginalDialogs.StandardButton.No,
            defaultButton: OriginalDialogs.StandardButton.Yes,
            title: "Delete",
            text: modalMessage
        });
        object.selected.connect(function(button) {
            if (button === OriginalDialogs.StandardButton.Yes) {
                doDeleteFile(paths);
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
            var browser = desktop.fileDialog({
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

    Item {
        width: pane.contentWidth
        height: pane.height

        // The letterbox used for popup messages
        LetterboxMessage {
            id: letterboxMessage;
            z: 999; // Force the popup on top of everything else
        }

        HifiControls.ContentSection {
            id: assetDirectory
            name: "Asset Directory"
            isFirst: true

            HifiControls.VerticalSpacer {}

            Row {
                anchors.left: parent.left
                anchors.right: parent.right
                spacing: hifi.dimensions.contentSpacing.x

                HifiControls.Button {
                    text: "Add To World"
                    color: hifi.buttons.blue
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
        }

        HifiControls.Tree {
            id: treeView
            anchors.top: assetDirectory.bottom
            anchors.bottom: infoRow.top
            anchors.bottomMargin: 2 * hifi.dimensions.contentSpacing.y
            anchors.margins: hifi.dimensions.contentMargin.x + 2  // Extra for border
            anchors.left: parent.left
            anchors.right: parent.right

            treeModel: assetProxyModel
            selectionMode: SelectionMode.ExtendedSelection
            headerVisible: true
            sortIndicatorVisible: true

            colorScheme: root.colorScheme

            modifyEl: renameEl

            TableViewColumn {
                id: nameColumn
                title: "Name:"
                role: "name"
                width: treeView.width - bakedColumn.width;
            }
            TableViewColumn {
                id: bakedColumn
                title: "Use Baked?"
                role: "baked"
                width: 170
            }

            onSortIndicatorOrderChanged: {
                Assets.sortProxyModel(sortIndicatorColumn, sortIndicatorOrder);
            }

            itemDelegate: Loader {
                id: itemDelegateLoader

                anchors {
                    left: parent ? parent.left : undefined
                    leftMargin: (styleData.column === 0 ? (2 + styleData.depth) : 1) * hifi.dimensions.tablePadding
                    right: parent ? parent.right : undefined
                    rightMargin: hifi.dimensions.tablePadding
                    verticalCenter: parent ? parent.verticalCenter : undefined
                }

                function convertToGlyph(text) {
                    switch (text) {
                        case "Not Baked":
                            return hifi.glyphs.circleSlash;
                        case "Baked":
                            return hifi.glyphs.checkmark;
                        case "Error":
                            return hifi.glyphs.alert;
                        default:
                            return "";
                    }
                }

                function getComponent() {
                    if ((styleData.column === 0) && styleData.selected) {
                        return textFieldComponent;
                    } else if (convertToGlyph(styleData.value) != "") {
                        return glyphComponent;
                    } else {
                        return labelComponent;
                    }

                }
                sourceComponent: getComponent()

                Component {
                    id: labelComponent
                    FiraSansSemiBold {
                        text: styleData.value
                        size: hifi.fontSizes.tableText
                        color: colorScheme == hifi.colorSchemes.light
                                ? (styleData.selected ? hifi.colors.black : hifi.colors.baseGrayHighlight)
                                : (styleData.selected ? hifi.colors.black : hifi.colors.lightGrayText)

                        horizontalAlignment: styleData.column === 1 ? TextInput.AlignHCenter : TextInput.AlignLeft

                        elide: Text.ElideMiddle

                        MouseArea {
                            id: mouseArea
                            anchors.fill: parent

                            acceptedButtons: Qt.NoButton
                            hoverEnabled: true

                            onEntered: {
                                if (parent.truncated) {
                                    treeLabelToolTip.show(parent);
                                }
                            }
                            onExited: treeLabelToolTip.hide();
                        }
                    }
                }
                Component {
                    id: glyphComponent

                    HiFiGlyphs {
                        text: convertToGlyph(styleData.value)
                        size: hifi.dimensions.frameIconSize
                        color: colorScheme == hifi.colorSchemes.light
                                ? (styleData.selected ? hifi.colors.black : hifi.colors.baseGrayHighlight)
                                : (styleData.selected ? hifi.colors.black : hifi.colors.lightGrayText)

                        elide: Text.ElideRight
                        horizontalAlignment: TextInput.AlignHCenter

                        HifiControls.ToolTip {
                            anchors.fill: parent

                            visible: styleData.value === "Error"

                            toolTip: assetProxyModel.data(styleData.index, 0x106)
                        }
                    }
                }
                Component {
                    id: textFieldComponent

                    TextField {
                        id: textField
                        readOnly: !activeFocus

                        text: styleData.value

                        font.family: "Fira Sans SemiBold"
                        font.pixelSize: hifi.fontSizes.textFieldInput
                        height: hifi.dimensions.tableRowHeight

                        style: TextFieldStyle {
                            textColor: readOnly
                                        ? hifi.colors.black
                                        : (treeView.isLightColorScheme ?  hifi.colors.black :  hifi.colors.white)
                            background: Rectangle {
                                visible: !readOnly
                                color: treeView.isLightColorScheme ? hifi.colors.white : hifi.colors.black
                                border.color: hifi.colors.primaryHighlight
                                border.width: 1
                            }
                            selectedTextColor: hifi.colors.black
                            selectionColor: hifi.colors.primaryHighlight
                            padding.left: readOnly ? 0 : hifi.dimensions.textPadding
                            padding.right: readOnly ? 0 : hifi.dimensions.textPadding
                        }

                        validator: RegExpValidator {
                            regExp: /[^/]+/
                        }

                        Keys.onPressed: {
                            if (event.key == Qt.Key_Escape) {
                                text = styleData.value;
                                unfocusHelper.forceActiveFocus();
                                event.accepted = true;
                            }
                        }
                        onAccepted:  {
                            if (acceptableInput && styleData.selected) {
                                if (!treeView.modifyEl(treeView.selection.currentIndex, text)) {
                                    text = styleData.value;
                                }
                                unfocusHelper.forceActiveFocus();
                            }
                        }

                        onReadOnlyChanged: {
                            // Have to explicily set keyboardRaised because automatic setting fails because readOnly is true at the time.
                            keyboardRaised = activeFocus;
                        }
                    }
                }
            }// End_OF( itemLoader )

            Rectangle {
                id: treeLabelToolTip
                visible: false
                z: 100 // Render on top

                width: toolTipText.width + 2 * hifi.dimensions.textPadding
                height: hifi.dimensions.tableRowHeight
                color: colorScheme == hifi.colorSchemes.light ? hifi.colors.tableRowLightOdd : hifi.colors.tableRowDarkOdd
                border.color: colorScheme == hifi.colorSchemes.light ? hifi.colors.black : hifi.colors.lightGrayText

                FiraSansSemiBold {
                    id: toolTipText
                    anchors.centerIn: parent

                    size: hifi.fontSizes.tableText
                    color: colorScheme == hifi.colorSchemes.light ? hifi.colors.black : hifi.colors.lightGrayText
                }

                Timer {
                    id: showTimer
                    interval: 1000
                    onTriggered: { treeLabelToolTip.visible = true; }
                }
                function show(item) {
                    var coord = item.mapToItem(parent, item.x, item.y);

                    toolTipText.text = item.text;
                    treeLabelToolTip.x = coord.x - hifi.dimensions.textPadding;
                    treeLabelToolTip.y = coord.y;
                    showTimer.start();
                }
                function hide() {
                    showTimer.stop();
                    treeLabelToolTip.visible = false;
                }
            }// End_OF( treeLabelToolTip )

            MouseArea {
                propagateComposedEvents: true
                anchors.fill: parent
                acceptedButtons: Qt.RightButton
                onClicked: {
                    if (treeView.selection.hasSelection && !HMD.active) {  // Popup only displays properly on desktop
                        // Only display the popup if the click triggered within
                        // the selection.
                        var clickedIndex = treeView.indexAt(mouse.x, mouse.y);
                        var displayContextMenu = false;
                        for ( var i = 0; i < selectedItemCount; ++i) {
                            var currentSelectedIndex = treeView.selection.selectedIndexes[i];
                            if (clickedIndex === currentSelectedIndex) {
                                contextMenu.popup();
                                break;
                            }
                        }
                    }
                }
            }

            Menu {
                id: contextMenu
                title: "Edit"
                property var url: ""

                MenuItem {
                    text: "Copy URL"
                    enabled: (selectedItemCount == 1)
                    onTriggered: {
                        copyURLToClipboard(treeView.selection.currentIndex);
                    }
                }

                MenuItem {
                    text: "Rename"
                    enabled: (selectedItemCount == 1)
                    onTriggered: {
                        renameFile(treeView.selection.currentIndex);
                    }
                }

                MenuItem {
                    text: "Delete"
                    enabled: (selectedItemCount > 0)
                    onTriggered: {
                        deleteFile();
                    }
                }
            }// End_OF( contextMenu )
        }// End_OF( treeView )

        Row {
            id: infoRow
            anchors.left: treeView.left
            anchors.right: treeView.right
            anchors.bottom: uploadSection.top

            RalewayRegular {
                anchors.verticalCenter: parent.verticalCenter

                function makeText() {
                    var numPendingBakes = assetMappingsModel.numPendingBakes;
                    if (selectedItemCount > 1 || numPendingBakes === 0) {
                        return selectedItemCount + " items selected";
                    } else {
                        return numPendingBakes + " bakes pending"
                    }
                }

                size: hifi.fontSizes.sectionName
                font.capitalization: Font.AllUppercase
                text: makeText()
                color: hifi.colors.lightGrayText
            }

            HifiControls.HorizontalSpacer { }

            HifiControls.CheckBox {
                id: bakingCheckbox
                anchors.leftMargin: 2 * hifi.dimensions.contentSpacing.x
                anchors.verticalCenter: parent.verticalCenter

                text: " Use baked version"
                colorScheme: root.colorScheme
                enabled: isEnabled()
                checked: isChecked()
                onClicked: {
                    var mappings = [];
                    for (var i in treeView.selection.selectedIndexes) {
                        var index = treeView.selection.selectedIndexes[i];
                        var path = assetProxyModel.data(index, 0x100);
                        mappings.push(path);
                    }
                    print("Setting baking enabled:" + mappings + " " + checked);
                    Assets.setBakingEnabled(mappings, checked, function() {
                        reload();
                    });

                    checked = Qt.binding(isChecked);
                }

                function getStatus() {
                    // kind of hack for ensuring getStatus() will be re-evaluated on updatesCount changes
                    return updatesCount, assetProxyModel.data(treeView.selection.currentIndex, 0x105);
                }
                
                function isEnabled() {
                    if (!treeView.selection.hasSelection) {
                        return false;
                    }

                    var status = getStatus();
                    if (status === "--") {
                        return false;
                    }
                    var bakingEnabled = status !== "Not Baked";

                    for (var i in treeView.selection.selectedIndexes) {
                        var thisStatus = assetProxyModel.data(treeView.selection.selectedIndexes[i], 0x105);
                        if (thisStatus === "--") {
                            return false;
                        }
                        var thisBakingEnalbed = (thisStatus !== "Not Baked");

                        if (bakingEnabled !== thisBakingEnalbed) {
                            return false;
                        }
                    }

                    return true;
                }
                function isChecked() {
                    if (!treeView.selection.hasSelection) {
                        return false;
                    }

                    var status = getStatus();
                    return isEnabled() && status !== "Not Baked"; 
                }  
            }

            Item {
                anchors.verticalCenter: parent.verticalCenter
                width: infoGlyph.size;
                height: infoGlyph.size;

                HiFiGlyphs {
                    id: infoGlyph;
                    anchors.fill: parent;
                    horizontalAlignment: Text.AlignHCenter;
                    verticalAlignment: Text.AlignVCenter;
                    text: hifi.glyphs.question;
                    size: 35;
                    color:  hifi.colors.lightGrayText;
                }
                MouseArea {
                    anchors.fill: parent;
                    hoverEnabled: true;
                    onEntered: infoGlyph.color = hifi.colors.blueHighlight;
                    onExited: infoGlyph.color =  hifi.colors.lightGrayText;
                    onClicked: letterbox(hifi.glyphs.question,
                                            "What is baking?",
                                            "Baking compresses and optimizes files for faster network transfer and display. We recommend you bake your content to reduce initial load times for your visitors.");
                    }
            }
        }// End_OF( infoRow )

        HifiControls.ContentSection {
            id: uploadSection
            name: "Upload A File"
            spacing: hifi.dimensions.contentSpacing.y
            anchors.bottom: parent.bottom
            height: 95

            Item {
                height: parent.height
                width: parent.width
                HifiControls.QueuedButton {
                    id: uploadButton
                    anchors.right: parent.right

                    text: "Choose File"
                    color: hifi.buttons.blue
                    colorScheme: root.colorScheme
                    height: 30
                    width: 155

                    onClickedQueued: uploadClicked()
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
                        source: "../../images/Loading-Outer-Ring.png"
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
                        source: "../../images/Loading-Inner-H.png"
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
        }// End_OF( uploadSection )
    }
}

