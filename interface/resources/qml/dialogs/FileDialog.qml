import QtQuick 2.0
import QtQuick.Controls 1.4
import Qt.labs.folderlistmodel 2.1
import Qt.labs.settings 1.0

import ".."
import "."

// Work in progress....
DialogBase {
    id: root
    Constants { id: vr }
    property string settingsName: ""

    signal selectedFile(var file);
    signal canceled();

    function selectCurrentFile() {
        var row = tableView.currentRow
        console.log("Selecting row " + row)
        var fileName = folderModel.get(row, "fileName");
        if (fileName === "..") {
            folderModel.folder = folderModel.parentFolder
        } else if (folderModel.isFolder(row)) {
            folderModel.folder = folderModel.get(row, "fileURL");
        } else {
            selectedFile(folderModel.get(row, "fileURL"));
            enabled = false
        }

    }

    property var folderModel: FolderListModel {
        id: folderModel
        showDotAndDotDot: true
        showDirsFirst: true
        folder: "file:///C:/";
        onFolderChanged: tableView.currentRow = 0
    }

    property var filterModel: ListModel {
        ListElement {
            text: "All Files (*.*)"
            filter: "*"
        }
        ListElement {
            text: "Javascript Files (*.js)"
            filter: "*.js"
        }
        ListElement {
            text: "QML Files (*.qml)"
            filter: "*.qml"
        }
    }

    Settings {
        // fixme, combine with a property to allow different saved locations
        category: "FileOpenLastFolder." + settingsName
        property alias folder: folderModel.folder
    }

    Rectangle {
        id: currentDirectoryWrapper
        anchors.left: parent.left
        anchors.leftMargin: 8
        anchors.right: parent.right
        anchors.rightMargin: 8
        anchors.top: parent.top
        anchors.topMargin: 8
        height: currentDirectory.implicitHeight + 8
        radius: vr.styles.radius
        color: vr.controls.colors.inputBackground

        TextEdit {
            enabled: false
            id: currentDirectory
            text: folderModel.folder
            anchors {
                fill: parent
                leftMargin: 8
                rightMargin: 8
                topMargin: 4
                bottomMargin: 4
            }
        }
    }


    TableView {
        id: tableView
        focus: true
        model: folderModel
        anchors.top: currentDirectoryWrapper.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: selectionType.top
        anchors.margins: 8
        itemDelegate: Item {
            Text {
                anchors.verticalCenter: parent.verticalCenter
                font.family: vr.fonts.lightFontName
                font.pointSize: 12
                color: tableView.activeFocus && styleData.row === tableView.currentRow ? "yellow" : styleData.textColor
                elide: styleData.elideMode
                text: getText();
                function getText() {
                    switch (styleData.column) {
                        //case 1: return Date.fromLocaleString(locale, styleData.value, "yyyy-MM-dd hh:mm:ss");
                        case 2: return folderModel.get(styleData.row, "fileIsDir") ? "" : formatSize(styleData.value);
                        default: return styleData.value;
                    }
                }

                function formatSize(size) {
                    var suffixes = [ "bytes", "KB", "MB", "GB", "TB", "PB", "EB", "ZB", "YB" ];
                    var suffixIndex = 0
                    while ((size / 1024.0) > 1.1) {
                        size /= 1024.0;
                        ++suffixIndex;
                    }

                    size = Math.round(size*1000)/1000;
                    size = size.toLocaleString()

                    return size + " " + suffixes[suffixIndex];
                }
            }
        }

        TableViewColumn {
            role: "fileName"
            title: "Name"
            width: 400
        }
        TableViewColumn {
            role: "fileModified"
            title: "Date Modified"
            width: 200
        }
        TableViewColumn {
            role: "fileSize"
            title: "Size"
            width: 200
        }

        function selectItem(row) {
            selectCurrentFile()
        }


        Keys.onReturnPressed: selectCurrentFile();
        onDoubleClicked: { currentRow = row; selectCurrentFile(); }
        onCurrentRowChanged: currentSelection.text = model.get(currentRow, "fileName");
        KeyNavigation.left: cancelButton
        KeyNavigation.right: selectionType
    }

    Rectangle {
        anchors.right: selectionType.left
        anchors.rightMargin: 8
        anchors.top: selectionType.top
        anchors.bottom: selectionType.bottom
        anchors.left: parent.left
        anchors.leftMargin: 8
        radius: 8
        color: "#eee"
        Text {
            id: currentSelection
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.margins: 8
            anchors.verticalCenter: parent.verticalCenter
            font.family: vr.fonts.mainFontName
            font.pointSize: 18
        }
    }

    ComboBox {
        id: selectionType
        anchors.bottom: buttonRow.top
        anchors.bottomMargin: 8
        anchors.right: parent.right
        anchors.rightMargin: 8
        anchors.left: buttonRow.left
        model: filterModel
        onCurrentIndexChanged: folderModel.nameFilters = [
                                   filterModel.get(currentIndex).filter
                               ]

        KeyNavigation.left: tableView
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
            KeyNavigation.right: tableView.contentItem
            Keys.onReturnPressed: { canceled(); root.enabled = false }
            onClicked: { canceled(); root.enabled = false }
        }
        Button {
            id: openButton
            text: "Open"
            enabled: tableView.currentRow != -1 && !folderModel.get(tableView.currentRow, "fileIsDir")
            KeyNavigation.up: selectionType
            KeyNavigation.left: selectionType
            KeyNavigation.right: cancelButton
            onClicked: selectCurrentFile();
            Keys.onReturnPressed: selectedFile(folderModel.get(tableView.currentRow, "fileURL"))
        }
    }

    Keys.onPressed: {
        if (event.key === Qt.Key_Backspace && folderModel.parentFolder && folderModel.parentFolder != "") {
            console.log("Navigating to " + folderModel.parentFolder)
            folderModel.folder = folderModel.parentFolder
            event.accepted = true
        }
    }
}


