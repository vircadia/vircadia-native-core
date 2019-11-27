import QtQuick 2.7
import QtQuick.Controls 2.3

StackView {
    id: editRoot
    objectName: "stack"

    signal sendToScript(var message);

    topPadding: 40
    leftPadding: 0
    rightPadding: 0
    bottomPadding: 0

    property var itemProperties: {"y": editRoot.topPadding,
                                  "width": editRoot.availableWidth,
                                  "height": editRoot.availableHeight }
    Component.onCompleted: {
        tab.currentIndex = 0
    }

    background: Rectangle {
        color: "#404040" //default background color
        EditTabView {
            id: tab
            anchors.fill: parent
            currentIndex: -1
            onCurrentIndexChanged: {
                editRoot.replace(null, tab.itemAt(currentIndex).visualItem,
                                 itemProperties,
                                 StackView.Immediate)
            }
        }
    }

    function pushSource(path) {
        var item = Qt.createComponent(path);
        editRoot.push(item, itemProperties,
                      StackView.Immediate);
        editRoot.currentItem.sendToScript.connect(editRoot.sendToScript);
    }

    function popSource() {
        editRoot.pop(StackView.Immediate);
    }

    // Passes script messages to the item on the top of the stack
    function fromScript(message) {
        var currentItem = editRoot.currentItem;
        if (currentItem && currentItem.fromScript)
            currentItem.fromScript(message);
    }

    Component.onDestruction: {
        if (KeyboardScriptingInterface.raised) {
            KeyboardScriptingInterface.raised = false;
        }
    }
}

