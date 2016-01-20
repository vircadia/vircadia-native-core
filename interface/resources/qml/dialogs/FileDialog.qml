import QtQuick 2.0
import QtQuick.Controls 1.4
import Qt.labs.folderlistmodel 2.1
import Qt.labs.settings 1.0

import ".."
import "../windows"
import "../styles"

// Work in progress....
Window {
    id: root
    HifiConstants { id: hifi }

    signal selectedFile(var file);
    signal canceled();

    anchors.centerIn: parent
    resizable: true
    width: 640
    height: 480
    modality: Qt.ApplicationModal
    property string settingsName: ""
    property alias folder: folderModel.folder
    property alias filterModel: selectionType.model


    Rectangle {
        anchors.fill: parent
        color: "white"

        Settings {
            // fixme, combine with a property to allow different saved locations
            category: "FileOpenLastFolder." + settingsName
            property alias folder: folderModel.folder
        }

        TextField {
            id: currentDirectory
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.margins: 8
            readOnly: true
            text: folderModel.folder
        }

        Component {
            id: fileItemDelegate
            Item {
                clip: true
                Text {
                    x: 3
                    id: columnText
                    anchors.verticalCenter: parent.verticalCenter
//                    font.pointSize: 12
                    color: tableView.activeFocus && styleData.row === tableView.currentRow ? "yellow" : styleData.textColor
                    elide: styleData.elideMode
                    text: getText();
                    font.italic: folderModel.get(styleData.row, "fileIsDir") ? true : false


                    Connections {
                        target: tableView
                        //onCurrentRowChanged: columnText.color = (tableView.activeFocus && styleData.row === tableView.currentRow ? "yellow" : styleData.textColor)
                    }

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
        }

        TableView {
            id: tableView
            focus: true
            model: FolderListModel {
                id: folderModel
                showDotAndDotDot: true
                showDirsFirst: true
                onFolderChanged: {
                    tableView.currentRow = -1;
                    tableView.positionViewAtRow(0, ListView.Beginning)
                }
            }
            anchors.top: currentDirectory.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: selectionType.top
            anchors.margins: 8
            itemDelegate: fileItemDelegate
//            rowDelegate: Rectangle {
//                id: rowDelegate
//                color: styleData.selected  ? "#7f0000ff" : "#00000000"
//                Connections { target: folderModel; onFolderChanged: rowDelegate.visible = false }
//                Connections { target: tableView;  onCurrentRowChanged: rowDelegate.visible = true; }
//            }

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

            Keys.onReturnPressed: selectCurrentFile();
            onDoubleClicked: { currentRow = row; selectCurrentFile(); }
            onCurrentRowChanged: currentSelection.text = model.get(currentRow, "fileName");
            KeyNavigation.left: cancelButton
            KeyNavigation.right: selectionType
        }


        TextField {
            id: currentSelection
            anchors.right: selectionType.left
            anchors.rightMargin: 8
            anchors.left: parent.left
            anchors.leftMargin: 8
            anchors.top: selectionType.top
        }

        ComboBox {
            id: selectionType
            anchors.bottom: buttonRow.top
            anchors.bottomMargin: 8
            anchors.right: parent.right
            anchors.rightMargin: 8
            anchors.left: buttonRow.left

            model: ListModel {
                ListElement { text: "All Files (*.*)"; filter: "*.*" }
            }

            onCurrentIndexChanged: {
                folderModel.nameFilters = [ filterModel.get(currentIndex).filter ]
            }
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
                onClicked: { canceled(); close() }
            }
            Button {
                id: openButton
                text: "Open"
                enabled: tableView.currentRow != -1 && !folderModel.get(tableView.currentRow, "fileIsDir")
                onClicked: selectCurrentFile();
                Keys.onReturnPressed: selectCurrentFile();

                KeyNavigation.up: selectionType
                KeyNavigation.left: selectionType
                KeyNavigation.right: cancelButton
            }
        }
    }

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
            close();
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


