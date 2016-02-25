import QtQuick 2.5
import QtQuick.Controls 1.4
import QtQuick.XmlListModel 2.0
import QtQuick.Controls.Styles 1.4

import "../../windows"
import "../../js/Utils.js" as Utils
import "../models"

ModalWindow {
    id: root
    resizable: true
    width: 640
    height: 480

    property var result;

    signal selected(var modelUrl);
    signal canceled();

    Rectangle {
        anchors.fill: parent
        color: "white"

        Item {
            anchors { fill: parent; margins: 8 }

            TextField {
                id: filterEdit
                anchors { left: parent.left; right: parent.right; top: parent.top }
                style:  TextFieldStyle { renderType: Text.QtRendering }
                placeholderText: "filter"
                onTextChanged: tableView.model.filter = text
            }

            TableView {
                id: tableView
                anchors { left: parent.left; right: parent.right; top: filterEdit.bottom; topMargin: 8; bottom: buttonRow.top; bottomMargin: 8 }
                model: S3Model{}
                onCurrentRowChanged: {
                    if (currentRow == -1) {
                        root.result = null;
                        return;
                    }
                    result = model.baseUrl + "/" + model.get(tableView.currentRow).key;
                }
                itemDelegate: Component {
                    Item {
                        clip: true
                        Text {
                            x: 3
                            anchors.verticalCenter: parent.verticalCenter
                            color: tableView.activeFocus && styleData.row === tableView.currentRow ? "yellow" : styleData.textColor
                            elide: styleData.elideMode
                            text: getText()

                            function getText() {
                                switch(styleData.column) {
                                    case 1:
                                        return Utils.formatSize(styleData.value)
                                    default:
                                        return styleData.value;
                                }
                            }

                        }
                    }
                }
                TableViewColumn {
                    role: "name"
                    title: "Name"
                    width: 200
                }
                TableViewColumn {
                    role: "size"
                    title: "Size"
                    width: 100
                }
                TableViewColumn {
                    role: "modified"
                    title: "Last Modified"
                    width: 200
                }
                Rectangle {
                    anchors.fill: parent
                    visible: tableView.model.status !== XmlListModel.Ready
                    color: "#7fffffff"
                    BusyIndicator {
                        anchors.centerIn: parent
                        width: 48; height: 48
                        running: true
                    }
                }
            }

            Row {
                id: buttonRow
                anchors { right: parent.right; bottom: parent.bottom }
                Button { action: acceptAction }
                Button { action: cancelAction }
            }

            Action {
                id: acceptAction
                text: qsTr("OK")
                enabled: root.result ? true : false
                shortcut: Qt.Key_Return
                onTriggered: {
                    root.selected(root.result);
                    root.destroy();
                }
            }

            Action {
                id: cancelAction
                text: qsTr("Cancel")
                shortcut: Qt.Key_Escape
                onTriggered: {
                    root.canceled();
                    root.destroy();
                }
            }
        }

    }
}




