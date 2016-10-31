import QtQuick 2.4

import QtQuick.Dialogs 1.1 as OriginalDialogs

import "../../qml/dialogs"

QtObject {
    id: root
    signal accepted;
    signal rejected;
    property var text;

    property var messageDialogBuilder: Component { MessageDialog { } }

    function open() {
        var dialog = messageDialogBuilder.createObject(desktop, {
            text: root.text,
            icon: OriginalDialogs.StandardIcon.Question,
            buttons: OriginalDialogs.StandardButton.Ok | OriginalDialogs.StandardButton.Cancel
        });

        dialog.selected.connect(function(button){
            if (button === OriginalDialogs.StandardButton.Ok) {
                accepted()
            } else {
                rejected();
            }
            dialog.destroy();
        });
    }
}
