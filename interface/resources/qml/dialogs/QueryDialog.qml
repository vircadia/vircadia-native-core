import QtQuick 2.5
import QtQuick.Controls 1.2
import QtQuick.Dialogs 1.2 as OriginalDialogs

import "../controls" as VrControls
import "../styles"
import "../windows"

ModalWindow {
    id: root
    HifiConstants { id: hifi }
    implicitWidth: 640
    implicitHeight: 320
    visible: true

    signal selected(var result);
    signal canceled();

    property alias result: textResult.text
    property alias placeholderText: textResult.placeholderText
    property alias text: mainTextContainer.text

    Rectangle {
        clip: true
        anchors.fill: parent
        radius: 4
        color: "white"

        QtObject {
            id: d
            readonly property real spacing: hifi.layout.spacing
            readonly property real outerSpacing: hifi.layout.spacing * 2
            readonly property int minWidth: 480
            readonly property int maxWdith: 1280
            readonly property int minHeight: 120
            readonly property int maxHeight: 720

            function resize() {
                var targetWidth = mainTextContainer.width + d.spacing * 6
                var targetHeight = mainTextContainer.implicitHeight + textResult.height + d.spacing  + buttons.height
                root.width = (targetWidth < d.minWidth) ? d.minWidth : ((targetWidth > d.maxWdith) ? d.maxWidth : targetWidth)
                root.height = (targetHeight < d.minHeight) ? d.minHeight: ((targetHeight > d.maxHeight) ? d.maxHeight : targetHeight)
            }
        }

        Text {
            id: mainTextContainer
            onHeightChanged: d.resize(); onWidthChanged: d.resize();
            wrapMode: Text.WordWrap
            font { pointSize: 14; weight: Font.Bold }
            anchors { left: parent.left; top: parent.top; margins: d.spacing }
        }

        Item {
            anchors { top: mainTextContainer.bottom; bottom: buttons.top; left: parent.left; right: parent.right; margins: d.spacing }
            // FIXME make a text field type that can be bound to a history for autocompletion
            TextField {
                focus: true
                id: textResult
                anchors { left: parent.left; right: parent.right; verticalCenter: parent.verticalCenter }
            }
        }

        Flow {
            id: buttons
            focus: true
            spacing: d.spacing
            onHeightChanged: d.resize(); onWidthChanged: d.resize();
            layoutDirection: Qt.RightToLeft
            anchors { bottom: parent.bottom; right: parent.right; margins: d.spacing; }
            Button { action: acceptAction }
            Button { action: cancelAction }
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
        Action {
            id: acceptAction
            text: qsTr("OK")
            shortcut: Qt.Key_Return
            onTriggered: {
                root.selected(root.result);
                root.destroy();
            }
        }
    }

    Keys.onPressed: {
        if (!visible) {
            return
        }

        switch (event.key) {
        case Qt.Key_Escape:
        case Qt.Key_Back:
            cancelAction.trigger()
            event.accepted = true;
            break;

        case Qt.Key_Return:
        case Qt.Key_Enter:
            acceptAction.trigger()
            event.accepted = true;
            break;
        }
    }
}
