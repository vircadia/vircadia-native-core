import QtQuick 2.5

import "."
import "../controls"

Frame {
    id: frame

    Item {
        anchors.fill: parent

        Rectangle {
            id: background
            anchors.fill: parent
            anchors.margins: -4096
            visible: window.visible
            color: "#7f7f7f7f";
            radius: 3;
        }

        Text {
            y: -implicitHeight - iconSize / 2
            text: window.title
            elide: Text.ElideRight
            font.bold: true
            color: window.focus ? "white" : "gray"
            style: Text.Outline;
            styleColor: "black"
        }
    }




}

