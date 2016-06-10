import QtQuick 2.5
import QtQuick.Controls 1.4
import QtQuick.Layouts 1.0

import "../../qml/controls-uit"
import "../../qml/styles-uit"
import "../../qml/dialogs"

QtObject {
    id: root
    signal input(string text);
    signal accepted;
    signal rejected;
    signal closing(var close)

    property var titleWidth;
    property var text;
    property var prompt;

    property var inputDialogBuilder: Component { QueryDialog { } }

    function open() {
        console.log("prompt text " + text)
        console.log("prompt prompt " + prompt)

        var dialog = inputDialogBuilder.createObject(desktop, {
            label: root.text,
            current: root.prompt
        });

        dialog.selected.connect(function(result){
            root.input(dialog.result)
            root.accepted();
            dialog.destroy();
        });

        dialog.canceled.connect(function(){
            root.rejected();
            dialog.destroy();
        });
    }
}
