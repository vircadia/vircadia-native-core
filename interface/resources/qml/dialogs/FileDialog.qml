import QtQuick 2.0
import QtQuick.Controls 1.4
import Qt.labs.folderlistmodel 2.1
import Qt.labs.settings 1.0

import ".."
import "../windows"
import "../styles"
import "../controls" as VrControls
import "fileDialog"

//FIXME implement shortcuts for favorite location
ModalWindow {
    id: root

    property bool selectDirectory: false;
    property bool showHidden: false;
    // FIXME implement
    property bool multiSelect: false;
    // FIXME implement
    property bool saveDialog: false;

    signal selectedFile(var file);
    signal canceled();
    resizable: true
    width: 640
    height: 480

    property var helper: fileDialogHelper
    property alias model: fileTableView.model
    property alias filterModel: selectionType.model
    property alias folder: model.folder

    Rectangle {
        anchors.fill: parent
        color: "white"

        Row {
            id: navControls
            anchors { left: parent.left; top: parent.top; margins: 8 }
            spacing: 8
            // FIXME implement back button
//            VrControls.FontAwesome {
//                id: backButton
//                text: "\uf0a8"
//                size: currentDirectory.height
//                enabled: d.backStack.length != 0
//                MouseArea { anchors.fill: parent; onClicked: d.navigateBack() }
//            }
            VrControls.FontAwesome {
                id: upButton
                text: "\uf0aa"
                size: currentDirectory.height
                color: enabled ? "black" : "gray"
                MouseArea { anchors.fill: parent; onClicked: d.navigateUp() }
            }
            VrControls.FontAwesome {
                id: homeButton
                property var destination: helper.home();
                visible: destination ? true : false
                text: "\uf015"
                size: currentDirectory.height
                MouseArea { anchors.fill: parent; onClicked: model.folder = parent.destination }
            }
        }

        TextField {
            id: currentDirectory
            anchors { left: navControls.right; right: parent.right; top: parent.top; margins: 8 }
            property var lastValidFolder: helper.urlToPath(model.folder)
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
                }
            }

            function navigateUp() {
                if (model.parentFolder && model.parentFolder !== "") {
                    model.folder = model.parentFolder
                    return true;
                }
            }
        }

        FileTableView {
            id: fileTableView
            anchors { left: parent.left; right: parent.right; top: currentDirectory.bottom; bottom: currentSelection.top; margins: 8 }
            onDoubleClicked: navigateToRow(row);
            model: FolderListModel {
                id: model
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
                    currentRow = -1;
                } else {
                    root.selectedFile(file);
                    root.visible = false;
                }
            }
        }

        TextField {
            id: currentSelection
            anchors { right: root.selectDirectory ? parent.right : selectionType.left; rightMargin: 8; left: parent.left; leftMargin: 8; top: selectionType.top }
            readOnly: true
        }

        ComboBox {
            id: selectionType
            anchors { bottom: buttonRow.top; bottomMargin: 8; right: parent.right; rightMargin: 8; left: buttonRow.left }
            visible: !selectDirectory
            model: ListModel { ListElement { text: "All Files (*.*)"; filter: "*.*" } }
//            onCurrentIndexChanged: model.nameFilters = [ filterModel.get(currentIndex).filter ]
            KeyNavigation.left: fileTableView
            KeyNavigation.right: openButton
        }

        Row {
            id: buttonRow
            anchors.right: parent.right
            anchors.rightMargin: 8
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 8
            layoutDirection: Qt.RightToLeft
            spacing: 8
            Button {
                id: cancelButton
                text: "Cancel"
                KeyNavigation.up: selectionType
                KeyNavigation.left: openButton
                KeyNavigation.right: fileTableView.contentItem
                Keys.onReturnPressed: { canceled(); root.enabled = false }
                onClicked: { canceled(); root.visible = false; }
            }
            Button {
                id: openButton
                text: root.selectDirectory ? "Choose" : "Open"
                enabled: currentSelection.text ? true : false
                onClicked: { selectedFile(d.currentSelectionUrl); root.visible = false; }
                Keys.onReturnPressed: { selectedFile(d.currentSelectionUrl); root.visible = false; }

                KeyNavigation.up: selectionType
                KeyNavigation.left: selectionType
                KeyNavigation.right: cancelButton
            }
        }
    }

    Keys.onPressed: {
        if (event.key === Qt.Key_Backspace && d.navigateUp()) {
            event.accepted = true
        }
    }
}


