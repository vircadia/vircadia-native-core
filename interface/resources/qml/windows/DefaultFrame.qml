import QtQuick 2.5

import "."
import "../controls"

Frame {
    id: frame
    // The frame fills the parent, which should be the size of the content.
    // The frame decorations use negative anchor margins to extend beyond 
    anchors.fill: parent
    // Size of the controls
    readonly property real iconSize: 24;

	// Convenience accessor for the window
    property alias window: frame.parent
    // FIXME needed?
    property alias decoration: decoration

    Rectangle {
        anchors { margins: -4 }
        visible: !decoration.visible
        anchors.fill: parent;
        color: "#7f7f7f7f";
        radius: 3;
    }

    Rectangle {
        id: decoration
        anchors { margins: -iconSize; topMargin: -iconSize * (window.closable ? 2 : 1); }
        // FIXME doesn't work
        // visible: window.activator.containsMouse
        anchors.fill: parent;
        color: "#7f7f7f7f";
        radius: 3;

        // Allow dragging of the window
        MouseArea {
            id: dragMouseArea
            anchors.fill: parent
            drag {
                target: window
    //            minimumX: (decoration.width - window.width) * -1
    //            minimumY: 0
    //            maximumX: (window.parent.width - window.width) - 2 * (decoration.width - window.width)
    //            maximumY: (window.parent.height - window.height) - 2 * (decoration.height - window.height)
            }
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
                visible: window.closable
                text: closeClickArea.containsMouse ? "\uf057" : "\uf05c"
                style: Text.Outline;
                styleColor: "white"
                color: closeClickArea.containsMouse ? "red" : "black"
                size: frame.iconSize
                MouseArea {
                    id: closeClickArea
                    anchors.fill: parent
                    hoverEnabled: true
                    onClicked: frame.close();
                }
            }
        }

        // Allow sizing of the window
        // FIXME works in native QML, doesn't work in Interface
        MouseArea {
            id: sizeDrag
            width: iconSize
            height: iconSize

            anchors {
                right: decoration.right;
                bottom: decoration.bottom
                bottomMargin: iconSize * 2
            }
            property vector2d pressOrigin
            property vector2d sizeOrigin
            property bool hid: false
            onPressed: {
                console.log("Pressed on size")
                pressOrigin = Qt.vector2d(mouseX, mouseY)
                sizeOrigin = Qt.vector2d(window.content.width, window.content.height)
                hid = false;
            }
            onReleased: {
                if (hid) {
                    window.content.visible = true
                    hid = false;
                }
            }

            onPositionChanged: {
                if (pressed) {
                    if (window.content.visible) {
                        window.content.visible = false;
                        hid = true;
                    }
                    var delta = Qt.vector2d(mouseX, mouseY).minus(pressOrigin);
                    frame.deltaSize(delta.x, delta.y)
                }
            }
        }

        FontAwesome {
            visible: window.resizable
            rotation: -45
            anchors { centerIn: sizeDrag }
            horizontalAlignment: Text.AlignHCenter
            text: "\uf07d"
            size: iconSize / 3 * 2
            style: Text.Outline; styleColor: "white"
        }
    }
}

