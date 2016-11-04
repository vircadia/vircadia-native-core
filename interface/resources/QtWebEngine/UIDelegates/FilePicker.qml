import QtQuick 2.4
import QtQuick.Dialogs 1.1
import QtQuick.Controls 1.4

import "../../qml/dialogs"

QtObject {
    id: root
    signal filesSelected(var fileList);
    signal rejected();
    property var text;
    property url fileUrl;
    property var fileUrls;
    property url folder;
    property var nameFilters;
    property bool selectExisting;
    property bool selectFolder;
    property bool selectMultiple;
    property string selectedNameFilter;
    property string title;

    property var fileDialogBuilder: Component { FileDialog { } }

    function open() {
        var foo = root;
        var dialog = fileDialogBuilder.createObject(desktop, {
        });

        dialog.canceled.connect(function(){
            root.filesSelected([]);
            dialog.destroy();
        });

        dialog.selectedFile.connect(function(file){
            root.filesSelected(fileDialogHelper.urlToList(file));
        });
    }
}

