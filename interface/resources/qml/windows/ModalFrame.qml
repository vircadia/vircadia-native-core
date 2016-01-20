import QtQuick 2.5

import "."
import "../controls"

Frame {
    id: frame
    // The frame fills the parent, which should be the size of the content.
    // The frame decorations use negative anchor margins to extend beyond 
    anchors.fill: parent
    property alias window: frame.parent

    Rectangle {
        anchors.fill: parent
        anchors.margins: -4096
        visible: window.visible
        color: "#7f7f7f7f";
        radius: 3;
        MouseArea {
            anchors.fill: parent
            acceptedButtons: Qt.AllButtons
            onClicked: { }
            onDoubleClicked: {}
            onPressAndHold: {}
            onReleased: {}
        }
    }
}

