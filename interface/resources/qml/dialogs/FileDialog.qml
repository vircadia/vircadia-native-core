//
//  FileDialog.qml
//
//  Created by Bradley Austin Davis on 14 Jan 2016
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import QtQuick 2.5
import QtQuick.Controls 1.4
import Qt.labs.folderlistmodel 2.1
import Qt.labs.settings 1.0
import QtQuick.Controls.Styles 1.4
import QtQuick.Dialogs 1.2 as OriginalDialogs

import ".."
import "../controls-uit"
import "../styles-uit"
import "../windows-uit"

import "fileDialog"

//FIXME implement shortcuts for favorite location
ModalWindow {
    id: root
    //resizable: true
    implicitWidth: 640
    implicitHeight: 480
    HifiConstants { id: hifi }

    Settings {
        category: "FileDialog"
        property alias width: root.width
        property alias height: root.height
        property alias x: root.x
        property alias y: root.y
    }


    // Set from OffscreenUi::getOpenFile()
    property alias caption: root.title;
    // Set from OffscreenUi::getOpenFile()
    property alias dir: model.folder;
    // Set from OffscreenUi::getOpenFile()
    property alias filter: selectionType.filtersString;
    // Set from OffscreenUi::getOpenFile()
    property int options; // <-- FIXME unused


    property bool selectDirectory: false;
    property bool showHidden: false;
    // FIXME implement
    property bool multiSelect: false;
    property bool saveDialog: false;
    property var helper: fileDialogHelper
    property alias model: fileTableView.model
    property var drives: helper.drives()

    property int titleWidth: 0

    signal selectedFile(var file);
    signal canceled();

    Component.onCompleted: {
        console.log("Helper " + helper + " drives " + drives)
        drivesSelector.onCurrentTextChanged.connect(function(){
            root.dir = helper.pathToUrl(drivesSelector.currentText);
        })
    }

    Item {
        clip: true
        width: pane.width
        height: pane.height
        anchors.margins: 0

        Row {
            id: navControls
            anchors {
                top: parent.top
                topMargin: hifi.dimensions.contentMargin.y
                left: parent.left
            }
            spacing: hifi.dimensions.contentSpacing.x

            // FIXME implement back button
            //VrControls.ButtonAwesome {
            //    id: backButton
            //    text: "\uf0a8"
            //    size: currentDirectory.height
            //    enabled: d.backStack.length != 0
            //    MouseArea { anchors.fill: parent; onClicked: d.navigateBack() }
            //}

            GlyphButton {
                id: upButton
                glyph: hifi.glyphs.levelUp
                width: height
                enabled: model.parentFolder && model.parentFolder !== ""
                onClicked: d.navigateUp();
            }

            GlyphButton {
                id: homeButton
                property var destination: helper.home();
                glyph: hifi.glyphs.home
                width: height
                enabled: d.homeDestination ? true : false
                onClicked: d.navigateHome();
            }

            ComboBox {
                id: drivesSelector
                width: 62
                model: drives
                visible: drives.length > 1
                currentIndex: 0
            }
        }

        TextField {
            id: currentDirectory
            property var lastValidFolder: helper.urlToPath(model.folder)
            height: homeButton.height
            anchors {
                top: parent.top
                topMargin: hifi.dimensions.contentMargin.y
                left: navControls.right
                leftMargin: hifi.dimensions.contentSpacing.x
                right: parent.right
            }
            onLastValidFolderChanged: text = lastValidFolder;

            // FIXME add support auto-completion
            onAccepted: {
                if (!helper.validFolder(text)) {
                    text = lastValidFolder;
                    return
                }
                model.folder = helper.pathToUrl(text);
            }
        }

        QtObject {
            id: d
            property var currentSelectionUrl;
            readonly property string currentSelectionPath: helper.urlToPath(currentSelectionUrl);
            property bool currentSelectionIsFolder;
            property var backStack: []
            property var tableViewConnection: Connections { target: fileTableView; onCurrentRowChanged: d.update(); }
            property var modelConnection: Connections { target: model; onFolderChanged: d.update(); }
            property var homeDestination: helper.home();
            Component.onCompleted: update();

            function update() {
                var row = fileTableView.currentRow;
                if (row === -1 && root.selectDirectory) {
                    currentSelectionUrl = fileTableView.model.folder;
                    currentSelectionIsFolder = true;
                    return;
                }

                currentSelectionUrl = fileTableView.model.get(row, "fileURL");
                currentSelectionIsFolder = fileTableView.model.isFolder(row);
                if (root.selectDirectory || !currentSelectionIsFolder) {
                    currentSelection.text = helper.urlToPath(currentSelectionUrl);
                } else {
                    currentSelection.text = ""
                }
            }

            function navigateUp() {
                if (model.parentFolder && model.parentFolder !== "") {
                    model.folder = model.parentFolder
                    return true;
                }
            }

            function navigateHome() {
                model.folder = homeDestination;
                return true;
            }
        }

        FileTableView {
            id: fileTableView
            anchors {
                top: navControls.bottom
                topMargin: hifi.dimensions.contentSpacing.y
                left: parent.left
                right: parent.right
                bottom: currentSelection.top
                bottomMargin: hifi.dimensions.contentSpacing.y + currentSelection.controlHeight - currentSelection.height
            }
            onDoubleClicked: navigateToRow(row);
            focus: true
            Keys.onReturnPressed: navigateToCurrentRow();
            Keys.onEnterPressed: navigateToCurrentRow();
            model: FolderListModel {
                id: model
                nameFilters: selectionType.currentFilter
                showDirsFirst: true
                showDotAndDotDot: false
                showFiles: !root.selectDirectory
                // For some reason, declaring these bindings directly in the targets doesn't
                // work for setting the initial state
                Component.onCompleted: {
                    currentDirectory.lastValidFolder  = Qt.binding(function() { return helper.urlToPath(model.folder); });
                    upButton.enabled = Qt.binding(function() { return (model.parentFolder && model.parentFolder != "") ? true : false; });
                    showFiles = !root.selectDirectory
                }
                onFolderChanged: {
                    fileTableView.selection.clear();
                    fileTableView.selection.select(0);
                    fileTableView.currentRow = 0;
                }
            }

            function navigateToRow(row) {
                currentRow = row;
                navigateToCurrentRow();
            }

            function navigateToCurrentRow() {
                var row = fileTableView.currentRow
                var isFolder = model.isFolder(row);
                var file = model.get(row, "fileURL");
                if (isFolder) {
                    fileTableView.model.folder = file
                } else {
                    okAction.trigger();
                }
            }

            property string prefix: ""

            function addToPrefix(event) {
                if (!event.text || event.text === "") {
                    return false;
                }
                var newPrefix = prefix + event.text.toLowerCase();
                var matchedIndex = -1;
                for (var i = 0; i < model.count; ++i) {
                    var name = model.get(i, "fileName").toLowerCase();
                    if (0 === name.indexOf(newPrefix)) {
                        matchedIndex = i;
                        break;
                    }
                }

                if (matchedIndex !== -1) {
                    fileTableView.selection.clear();
                    fileTableView.selection.select(matchedIndex);
                    fileTableView.currentRow = matchedIndex;
                    fileTableView.prefix = newPrefix;
                }
                prefixClearTimer.restart();
                return true;
            }

            Timer {
                id: prefixClearTimer
                interval: 1000
                repeat: false
                running: false
                onTriggered: fileTableView.prefix = "";
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
            label: "Path:"
            anchors {
                left: parent.left
                right: selectionType.visible ? selectionType.left: parent.right
                rightMargin: selectionType.visible ? hifi.dimensions.contentSpacing.x : 0
                bottom: buttonRow.top
                bottomMargin: hifi.dimensions.contentSpacing.y
            }
            readOnly: !root.saveDialog
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
            KeyNavigation.left: fileTableView
            KeyNavigation.right: openButton
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
                KeyNavigation.right: fileTableView.contentItem
                Keys.onReturnPressed: { canceled(); root.enabled = false }
            }
        }

        Action {
            id: okAction
            text: root.saveDialog ? "Save" : (root.selectDirectory ? "Choose" : "Open")
            enabled: currentSelection.text ? true : false
            onTriggered: okActionTimer.start();
        }

        Timer {
            id: okActionTimer
            interval: 50
            running: false
            repeat: false
            onTriggered: {
                if (!root.saveDialog) {
                    selectedFile(d.currentSelectionUrl);
                    root.destroy()
                    return;
                }


                // Handle the ambiguity between different cases
                // * typed name (with or without extension)
                // * full path vs relative vs filename only
                var selection = helper.saveHelper(currentSelection.text, root.dir, selectionType.currentFilter);

                if (!selection) {
                    desktop.messageBox({ icon: OriginalDialogs.StandardIcon.Warning, text: "Unable to parse selection" })
                    return;
                }

                if (helper.urlIsDir(selection)) {
                    root.dir = selection;
                    currentSelection.text = "";
                    return;
                }

                // Check if the file is a valid target
                if (!helper.urlIsWritable(selection)) {
                    desktop.messageBox({
                                           icon: OriginalDialogs.StandardIcon.Warning,
                                           buttons: OriginalDialogs.StandardButton.Yes | OriginalDialogs.StandardButton.No,
                                           text: "Unable to write to location " + selection
                                       })
                    return;
                }

                if (helper.urlExists(selection)) {
                    var messageBox = desktop.messageBox({
                                                            icon: OriginalDialogs.StandardIcon.Question,
                                                            buttons: OriginalDialogs.StandardButton.Yes | OriginalDialogs.StandardButton.No,
                                                            text: "Do you wish to overwrite " + selection + "?",
                                                        });
                    var result = messageBox.exec();
                    if (OriginalDialogs.StandardButton.Yes !== result) {
                        return;
                    }
                }

                console.log("Selecting " + selection)
                selectedFile(selection);
                root.destroy();
            }
        }

        Action {
            id: cancelAction
            text: "Cancel"
            onTriggered: { canceled(); root.visible = false; }
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
