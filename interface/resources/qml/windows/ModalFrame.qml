import QtQuick 2.5

import "."
import "../controls"

Frame {
    id: frame

    Rectangle {
        anchors.fill: parent
        anchors.margins: -4096
        visible: window.visible
        color: "#7f7f7f7f";
        radius: 3;
    }
}

