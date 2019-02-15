import QtQuick 2.2
import QtQuick.Dialogs 1.1
import Qt.labs.folderlistmodel 2.11

Item {
    id: root
    width: 640
    height: 480
    
    ListView {
        anchors.fill: parent

        FolderListModel {
            id: folderModel
            folder: FRAMES_FOLDER
            nameFilters: ["*.json"]
        }

        Component {
            id: fileDelegate
            Text { 
                text: fileName 
                font.pointSize: 36
                MouseArea {
                    anchors.fill: parent
                    onClicked: root.loadFile(folderModel.folder + "/" + fileName);
                }
            }
        }

        model: folderModel
        delegate: fileDelegate
    }

    signal loadFile(string filename);
}
