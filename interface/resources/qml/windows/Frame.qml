import QtQuick 2.5

import "../controls"
import "../js/Utils.js" as Utils

Item {
    id: frame
    // Frames always fill their parents, but their decorations may extend
    // beyond the window via negative margin sizes
    anchors.fill: parent

    // Convenience accessor for the window
    property alias window: frame.parent
    readonly property int iconSize: 24
    default property var decoration;

    children: [
        decoration,
        sizeOutline,
        debugZ,
        sizeDrag,
    ]

    Text {
        id: debugZ
        visible: DebugQML
        text: window ? "Z: " + window.z : ""
        y: -height
    }

    function deltaSize(dx, dy) {
        var newSize = Qt.vector2d(window.width + dx, window.height + dy);
        newSize = Utils.clampVector(newSize, window.minSize, window.maxSize);
        window.width = newSize.x
        window.height = newSize.y
    }

    Rectangle {
        id: sizeOutline
        width: window ? window.width : 0
        height: window ? window.height : 0
        color: "#00000000"
        border.width: 4
        radius: 10
        visible: window ? !window.content.visible : false
    }

    MouseArea {
        id: sizeDrag
        width: iconSize
        height: iconSize
        enabled: window ? window.resizable : false
        x: window ? window.width : 0
        y: window ? window.height : 0
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
        FontAwesome {
            visible: sizeDrag.enabled
            rotation: -45
            anchors { centerIn: parent }
            horizontalAlignment: Text.AlignHCenter
            text: "\uf07d"
            size: iconSize / 3 * 2
            style: Text.Outline; styleColor: "white"
        }
    }

}
