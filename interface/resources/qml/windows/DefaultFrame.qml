import QtQuick 2.5

import "."
import "../controls"

Frame {
    id: frame

    Rectangle {
        anchors { margins: -iconSize; topMargin: -iconSize * ((window && window.closable) ? 2 : 1); }
        anchors.fill: parent;
        color: "#7f7f7f7f";
        radius: 3;

        // Allow dragging of the window
        MouseArea {
            anchors.fill: parent
            drag.target: window
        }

        Row {
            id: controlsRow
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.rightMargin: iconSize
            anchors.topMargin: iconSize / 2
            spacing: iconSize / 4

            FontAwesome {
                visible: false
                text: "\uf08d"
                style: Text.Outline; styleColor: "white"
                size: frame.iconSize
                rotation: !frame.parent ? 90 : frame.parent.pinned ? 0 : 90
                color: frame.pinned ? "red" : "black"
                MouseArea {
                    anchors.fill: parent
                    propagateComposedEvents: true
                    onClicked: { frame.pin(); mouse.accepted = false; }
                }
            }
            FontAwesome {
                visible: window ? window.closable : false
                text: closeClickArea.containsMouse ? "\uf057" : "\uf05c"
                style: Text.Outline;
                styleColor: "white"
                color: closeClickArea.containsMouse ? "red" : "black"
                size: frame.iconSize
                MouseArea {
                    id: closeClickArea
                    anchors.fill: parent
                    hoverEnabled: true
                    onClicked: window.visible = false;
                }
            }
        }
    }
}

