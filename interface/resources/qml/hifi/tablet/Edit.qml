import QtQuick 2.7
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3
import "../../styles-uit"
import "../../controls-uit" as HifiControls
import "../../controls"
import "../toolbars"

StackView {
    id: editRoot
    objectName: "stack"
    //anchors.fill: parent
    topPadding: 40

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
            currentIndex: -1
            onCurrentIndexChanged: {
                editRoot.replace(null, tab.itemAt(currentIndex).visualItem,
                                 itemProperties,
                                 StackView.Immediate)
            }
        }
    }

    signal sendToScript(var message);

    function pushSource(path) {
        editRoot.push(Qt.resolvedUrl("../../" + path), itemProperties,
                      StackView.Immediate);
        editRoot.currentItem.sendToScript.connect(editRoot.sendToScript);
    }

    function popSource() {
        editRoot.pop(StackView.Immediate);
    }
}

