import QtQuick 2.5

import "../controls"

Item {
    id: frame

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
        text: "Z: " + window.z
        y: -height
    }

    function close() {
        window.close();
    }

    function raise() {
        window.raise();
    }

    function deltaSize(dx, dy) {
        var newSize = Qt.vector2d(window.width + dx, window.height + dy);
        newSize = clampVector(newSize, window.minSize, window.maxSize);
        window.width = newSize.x
        window.height = newSize.y
    }

    Rectangle {
        id: sizeOutline
        width: window.width
        height: window.height
        color: "#00000000"
        border.width: 4
        radius: 10
        visible: !window.content.visible
    }

    MouseArea {
        id: sizeDrag
        width: iconSize
        height: iconSize
        enabled: window.resizable
        x: window.width
        y: window.height
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
