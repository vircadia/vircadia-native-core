import QtQuick 2.5

import "."
import "../controls"

Frame {
    id: root
    anchors { margins: -16; topMargin: parent.closable ? -32 : -16; }


    MouseArea {
        id: controlsArea
        anchors.fill: desktopControls
        hoverEnabled: true
        drag.target: root.parent
        propagateComposedEvents: true
        onClicked: {
            root.raise()
            mouse.accepted = false;
        }

        MouseArea {
            id: sizeDrag
            enabled: root.parent.resizable
            property int startX
            property int startY
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            width: 16
            height: 16
            z: 1000
            hoverEnabled: true
            onPressed: {
                startX = mouseX
                startY = mouseY
            }
            onPositionChanged: {
                if (pressed) {
                    root.deltaSize((mouseX - startX), (mouseY - startY))
                    startX = mouseX
                    startY = mouseY
                }
            }
        }
    }

    Rectangle {
        id: desktopControls
        // FIXME this doesn't work as expected
        visible: root.parent.showFrame
        anchors.fill: parent;
        color: "#7f7f7f7f";
        radius: 3;


        Row {
            anchors.right: parent.right
            anchors.top: parent.top
            anchors.rightMargin: 4
            anchors.topMargin: 4
            spacing: 4
            FontAwesome {
                visible: false
                text: "\uf08d"
                style: Text.Outline; styleColor: "white"
                size: 16
                rotation: !root.parent ? 90 : root.parent.pinned ? 0 : 90
                color: root.pinned ? "red" : "black"
                MouseArea {
                    anchors.fill: parent
                    propagateComposedEvents: true
                    onClicked: { root.pin(); mouse.accepted = false; }
                }
            }

            FontAwesome {
                visible: root.parent.closable
                text: closeClickArea.containsMouse ? "\uf057" : "\uf05c"
                style: Text.Outline;
                styleColor: "white"
                color: closeClickArea.containsMouse ? "red" : "black"
                size: 16
                MouseArea {
                    id: closeClickArea
                    anchors.fill: parent
                    hoverEnabled: true
                    onClicked: root.close();
                }
            }
        }
    }
}

