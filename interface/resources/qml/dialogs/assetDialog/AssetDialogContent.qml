//
//  AssetDialogContent.qml
//
//  Created by David Rowe on 19 Apr 2017
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import QtQuick 2.5
import QtQuick.Controls 1.4

import "../../controls-uit"
import "../../styles-uit"

import "../fileDialog"

Item {
    // Set from OffscreenUi::assetDialog()
    property alias dir: assetTableModel.folder
    property alias filter: selectionType.filtersString  // FIXME: Currently only supports simple filters, "*.xxx".
    property int options  // Not used.

    property bool selectDirectory: false

    // Not implemented.
    //property bool saveDialog: false;
    //property bool multiSelect: false;

    property bool singleClickNavigate: false

    HifiConstants { id: hifi }

    Component.onCompleted: {
        homeButton.destination = dir;

        if (selectDirectory) {
            d.currentSelectionIsFolder = true;
            d.currentSelectionPath = assetTableModel.folder;
        }

        assetTableView.forceActiveFocus();
    }

    Item {
        id: assetDialogItem
        anchors.fill: parent
        clip: true

        MouseArea {
            // Clear selection when click on internal unused area.
            anchors.fill: parent
            drag.target: root
            onClicked: {
                d.clearSelection();
                frame.forceActiveFocus();
                assetTableView.forceActiveFocus();
            }
        }

        Row {
            id: navControls
            anchors {
                top: parent.top
                topMargin: hifi.dimensions.contentMargin.y
                left: parent.left
            }
            spacing: hifi.dimensions.contentSpacing.x

            GlyphButton {
                id: upButton
                glyph: hifi.glyphs.levelUp
                width: height
                size: 30
                enabled: assetTableModel.parentFolder !== ""
                onClicked: d.navigateUp();
            }

            GlyphButton {
                id: homeButton
                property string destination: ""
                glyph: hifi.glyphs.home
                size: 28
                width: height
                enabled: destination !== ""
                //onClicked: d.navigateHome();
                onClicked: assetTableModel.folder = destination;
            }
        }

        ComboBox {
            id: pathSelector
            anchors {
                top: parent.top
                topMargin: hifi.dimensions.contentMargin.y
                left: navControls.right
                leftMargin: hifi.dimensions.contentSpacing.x
                right: parent.right
            }
            z: 10

            property string lastValidFolder: assetTableModel.folder

            function calculatePathChoices(folder) {
                var folders = folder.split("/"),
                    choices = [],
                    i, length;

                if (folders[folders.length - 1] === "") {
                    folders.pop();
                }

                choices.push(folders[0]);

                for (i = 1, length = folders.length; i < length; i++) {
                    choices.push(choices[i - 1] + "/" + folders[i]);
                }

                if (folders[0] === "") {
                    choices[0] = "/";
                }

                choices.reverse();

                if (choices.length > 0) {
                    pathSelector.model = choices;
                }
            }

            onLastValidFolderChanged: {
                var folder = lastValidFolder;
                calculatePathChoices(folder);
            }

            onCurrentTextChanged: {
                var folder = currentText;

                if (folder !== "/") {
                    folder += "/";
                }

                if (folder !== assetTableModel.folder) {
                    if (root.selectDirectory) {
                        currentSelection.text = currentText;
                        d.currentSelectionPath = currentText;
                    }
                    assetTableModel.folder = folder;
                    assetTableView.forceActiveFocus();
                }
            }
        }

        QtObject {
            id: d

            property string currentSelectionPath
            property bool currentSelectionIsFolder
            property var tableViewConnection: Connections { target: assetTableView; onCurrentRowChanged: d.update(); }

            function update() {
                var row = assetTableView.currentRow;

                if (row === -1) {
                    if (!root.selectDirectory) {
                        currentSelection.text = "";
                        currentSelectionIsFolder = false;
                    }
                    return;
                }

                var rowInfo = assetTableModel.get(row);
                currentSelectionPath = rowInfo.filePath;
                currentSelectionIsFolder = rowInfo.fileIsDir;
                if (root.selectDirectory || !currentSelectionIsFolder) {
                    currentSelection.text = currentSelectionPath;
                } else {
                    currentSelection.text = "";
                }
            }

            function navigateUp() {
                if (assetTableModel.parentFolder !== "") {
                    assetTableModel.folder = assetTableModel.parentFolder;
                    return true;
                }
                return false;
            }

            function navigateHome() {
                assetTableModel.folder = homeButton.destination;
                return true;
            }

            function clearSelection() {
                assetTableView.selection.clear();
                assetTableView.currentRow = -1;
                update();
            }
        }

        ListModel {
            id: assetTableModel

            property string folder
            property string parentFolder: ""
            readonly property string rootFolder: "/"

            onFolderChanged: {
                parentFolder = calculateParentFolder();
                update();
            }

            function calculateParentFolder() {
                if (folder !== "/") {
                    return folder.slice(0, folder.slice(0, -1).lastIndexOf("/") + 1);
                }
                return "";
            }

            function isFolder(row) {
                if (row === -1) {
                    return false;
                }
                return get(row).fileIsDir;
            }

            function onGetAllMappings(error, map) {
                var mappings,
                    fileTypeFilter,
                    index,
                    path,
                    fileName,
                    fileType,
                    fileIsDir,
                    isValid,
                    subDirectory,
                    subDirectories = [],
                    fileNameSort,
                    rows = 0,
                    lower,
                    middle,
                    upper,
                    i,
                    length;

                clear();

                if (error === "") {
                    mappings = Object.keys(map);
                    fileTypeFilter = filter.replace("*", "").toLowerCase();

                    for (i = 0, length = mappings.length; i < length; i++) {
                        index = mappings[i].lastIndexOf("/");

                        path = mappings[i].slice(0, mappings[i].lastIndexOf("/") + 1);
                        fileName = mappings[i].slice(path.length);
                        fileType = fileName.slice(fileName.lastIndexOf("."));
                        fileIsDir = false;
                        isValid = false;

                        if (fileType.toLowerCase() === fileTypeFilter) {
                            if (path === folder) {
                                isValid = !selectDirectory;
                            } else if (path.length > folder.length) {
                                subDirectory = path.slice(folder.length);
                                index = subDirectory.indexOf("/");
                                if (index === subDirectory.lastIndexOf("/")) {
                                    fileName = subDirectory.slice(0, index);
                                    if (subDirectories.indexOf(fileName) === -1) {
                                        fileIsDir = true;
                                        isValid = true;
                                        subDirectories.push(fileName);
                                    }
                                }
                            }
                        }

                        if (isValid) {
                            fileNameSort = (fileIsDir ? "*" : "") + fileName.toLowerCase();

                            lower = 0;
                            upper = rows;
                            while (lower < upper) {
                                middle = Math.floor((lower + upper) / 2);
                                var lessThan;
                                if (fileNameSort < get(middle)["fileNameSort"]) {
                                    lessThan = true;
                                    upper = middle;
                                } else {
                                    lessThan = false;
                                    lower = middle + 1;
                                }
                            }

                            insert(lower, {
                               fileName: fileName,
                               filePath: path + (fileIsDir ? "" : fileName),
                               fileIsDir: fileIsDir,
                               fileNameSort: fileNameSort
                            });

                            rows++;
                        }
                    }

                } else {
                    console.log("Error getting mappings from Asset Server");
                }
            }

            function update() {
                d.clearSelection();
                clear();
                Assets.getAllMappings(onGetAllMappings);
            }
        }

        Table {
            id: assetTableView
            colorScheme: hifi.colorSchemes.light
            anchors {
                top: navControls.bottom
                topMargin: hifi.dimensions.contentSpacing.y
                left: parent.left
                right: parent.right
                bottom: currentSelection.top
                bottomMargin: hifi.dimensions.contentSpacing.y + currentSelection.controlHeight - currentSelection.height
            }

            model: assetTableModel

            focus: true

            onClicked: {
                if (singleClickNavigate) {
                    navigateToRow(row);
                }
            }

            onDoubleClicked: navigateToRow(row);
            Keys.onReturnPressed: navigateToCurrentRow();
            Keys.onEnterPressed: navigateToCurrentRow();

            itemDelegate: Item {
                clip: true

                FontLoader { id: firaSansSemiBold; source: "../../../fonts/FiraSans-SemiBold.ttf"; }
                FontLoader { id: firaSansRegular; source: "../../../fonts/FiraSans-Regular.ttf"; }

                FiraSansSemiBold {
                    text: styleData.value
                    elide: styleData.elideMode
                    anchors {
                        left: parent.left
                        leftMargin: hifi.dimensions.tablePadding
                        right: parent.right
                        rightMargin: hifi.dimensions.tablePadding
                        verticalCenter: parent.verticalCenter
                    }
                    size: hifi.fontSizes.tableText
                    color: hifi.colors.baseGrayHighlight
                    font.family: (styleData.row !== -1 && assetTableView.model.get(styleData.row).fileIsDir)
                        ? firaSansSemiBold.name : firaSansRegular.name
                }
            }

            TableViewColumn {
                id: fileNameColumn
                role: "fileName"
                title: "Name"
                width: assetTableView.width
                movable: false
                resizable: false
            }

            function navigateToRow(row) {
                currentRow = row;
                navigateToCurrentRow();
            }

            function navigateToCurrentRow() {
                if (model.isFolder(currentRow)) {
                    model.folder = model.get(currentRow).filePath;
                } else {
                    okAction.trigger();
                }
            }

            Timer {
                id: prefixClearTimer
                interval: 1000
                repeat: false
                running: false
                onTriggered: assetTableView.prefix = "";
            }

            property string prefix: ""

            function addToPrefix(event) {
                if (!event.text || event.text === "") {
                    return false;
                }
                var newPrefix = prefix + event.text.toLowerCase();
                var matchedIndex = -1;
                for (var i = 0; i < model.count; ++i) {
                    var name = model.get(i).fileName.toLowerCase();
                    if (0 === name.indexOf(newPrefix)) {
                        matchedIndex = i;
                        break;
                    }
                }

                if (matchedIndex !== -1) {
                    assetTableView.selection.clear();
                    assetTableView.selection.select(matchedIndex);
                    assetTableView.currentRow = matchedIndex;
                    assetTableView.prefix = newPrefix;
                }
                prefixClearTimer.restart();
                return true;
            }

            Keys.onPressed: {
                switch (event.key) {
                case Qt.Key_Backspace:
                case Qt.Key_Tab:
                case Qt.Key_Backtab:
                    event.accepted = false;
                    break;

                default:
                    if (addToPrefix(event)) {
                        event.accepted = true
                    } else {
                        event.accepted = false;
                    }
                    break;
                }
            }
        }

        TextField {
            id: currentSelection
            label: selectDirectory ? "Directory:" : "File name:"
            anchors {
                left: parent.left
                right: selectionType.visible ? selectionType.left: parent.right
                rightMargin: selectionType.visible ? hifi.dimensions.contentSpacing.x : 0
                bottom: buttonRow.top
                bottomMargin: hifi.dimensions.contentSpacing.y
            }
            readOnly: true
            activeFocusOnTab: !readOnly
            onActiveFocusChanged: if (activeFocus) { selectAll(); }
            onAccepted: okAction.trigger();
        }

        FileTypeSelection {
            id: selectionType
            anchors {
                top: currentSelection.top
                left: buttonRow.left
                right: parent.right
            }
            visible: !selectDirectory && filtersCount > 1
            KeyNavigation.left: assetTableView
            KeyNavigation.right: openButton
        }

        Action {
            id: okAction
            text: currentSelection.text && root.selectDirectory && assetTableView.currentRow === -1 ? "Choose" : "Open"
            enabled: currentSelection.text || !root.selectDirectory && d.currentSelectionIsFolder ? true : false
            onTriggered: {
                if (!root.selectDirectory && !d.currentSelectionIsFolder
                        || root.selectDirectory && assetTableView.currentRow === -1) {
                    selectedAsset(d.currentSelectionPath);
                    root.destroy();
                } else {
                    assetTableView.navigateToCurrentRow();
                }
            }
        }

        Action {
            id: cancelAction
            text: "Cancel"
            onTriggered: {
                canceled();
                root.destroy();
            }
        }

        Row {
            id: buttonRow
            anchors {
                right: parent.right
                bottom: parent.bottom
            }
            spacing: hifi.dimensions.contentSpacing.y

            Button {
                id: openButton
                color: hifi.buttons.blue
                action: okAction
                Keys.onReturnPressed: okAction.trigger()
                KeyNavigation.up: selectionType
                KeyNavigation.left: selectionType
                KeyNavigation.right: cancelButton
            }

            Button {
                id: cancelButton
                action: cancelAction
                KeyNavigation.up: selectionType
                KeyNavigation.left: openButton
                KeyNavigation.right: assetTableView.contentItem
                Keys.onReturnPressed: { canceled(); root.enabled = false }
            }
        }
    }

    Keys.onPressed: {
        switch (event.key) {
        case Qt.Key_Backspace:
            event.accepted = d.navigateUp();
            break;

        case Qt.Key_Home:
            event.accepted = d.navigateHome();
            break;

        }
    }
}
