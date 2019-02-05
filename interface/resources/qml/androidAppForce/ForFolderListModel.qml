import QtQuick 2.2
import QtQuick.Dialogs 1.1
import Qt.labs.folderlistmodel 2.11

Item {
    width: 640
    height: 480
    
    ListView {
        width: 200; height: 400

        FolderListModel {
            id: folderModel
            folder: "assets:/frames/"
            nameFilters: ["*.json"]
        }

        Component {
            id: fileDelegate
            Text { text: fileName }
        }

        model: folderModel
        delegate: fileDelegate
    }
}
