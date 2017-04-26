import QtQuick 2.5
import QtQuick.Controls 1.4
import "../../styles-uit"

StackView {
    id: editRoot
    objectName: "stack"
    initialItem: Qt.resolvedUrl('EditTabView.qml')

    property var eventBridge;
    signal sendToScript(var message);

    HifiConstants { id: hifi }

    function pushSource(path) {
        editRoot.push(Qt.resolvedUrl(path));
        editRoot.currentItem.eventBridge = editRoot.eventBridge;
        editRoot.currentItem.sendToScript.connect(editRoot.sendToScript);
    }

    function popSource() {
        editRoot.pop();
    }

    // Passes script messages to the item on the top of the stack
    function fromScript(message) {
        var currentItem = editRoot.currentItem;
        if (currentItem && currentItem.fromScript)
            currentItem.fromScript(message);
    }
}
