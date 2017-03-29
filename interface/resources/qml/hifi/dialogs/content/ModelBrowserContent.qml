import QtQuick 2.5
import QtQuick.Controls 1.4

import "../../../controls-uit" as HifiControls

Column {
    width: pane.contentWidth

    Rectangle {
        width: parent.width
        height: root.height  - (keyboardEnabled && keyboardRaised ? 200 : 0)
        color: hifi.colors.baseGray

        HifiControls.TextField {
            id: filterEdit
            anchors { left: parent.left; right: parent.right; top: parent.top ; margins: 8}
            placeholderText: "filter"
            onTextChanged: tableView.model.filter = text
            colorScheme: hifi.colorSchemes.dark
        }

        HifiControls.AttachmentsTable {
            id: tableView
            anchors { left: parent.left; right: parent.right; top: filterEdit.bottom; bottom: buttonRow.top; margins: 8; }
            colorScheme: hifi.colorSchemes.dark
            onCurrentRowChanged: {
                if (currentRow == -1) {
                    root.result = null;
                    return;
                }
                result = model.baseUrl + "/" + model.get(tableView.currentRow).key;
            }
        }

        Row {
            id: buttonRow
            spacing: 8
            anchors { right: parent.right; rightMargin: 8; bottom: parent.bottom; bottomMargin: 8; }
            HifiControls.Button { action: acceptAction ;  color: hifi.buttons.black; colorScheme: hifi.colorSchemes.dark }
            HifiControls.Button { action: cancelAction ;  color: hifi.buttons.black; colorScheme: hifi.colorSchemes.dark }
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
