import QtQuick 2.0

Item {
    id: keyItem
    width: 45
    height: 50
    property string glyph: "a"
    property bool toggle: false   // does this button have the toggle behaivor?
    property bool toggled: false  // is this button currently toggled?

    MouseArea {
        id: mouseArea1
        anchors.fill: parent
        hoverEnabled: true

        onCanceled: {
            keyItem.state = ""
        }

        onClicked: {
            mouse.accepted = true
            webEntity.synthesizeKeyPress(glyph)
            if (toggle) {
                toggled = !toggled
            }
        }

        onDoubleClicked: {
            mouse.accepted = true
        }

        onEntered: {
            keyItem.state = "mouseOver"
        }

        onExited: {
            if (toggled) {
                keyItem.state = "mouseDepressed"
            } else {
                keyItem.state = ""
            }
        }

        onPressed: {
            keyItem.state = "mouseClicked"
            mouse.accepted = true
        }

        onReleased: {
            if (containsMouse) {
                keyItem.state = "mouseOver"
            } else {
                if (toggled) {
                    keyItem.state = "mouseDepressed"
                } else {
                    keyItem.state = ""
                }
            }
            mouse.accepted = true
        }
    }

    Rectangle {
        id: roundedRect
        color: "#e2e2e2"
        radius: 10
        anchors.right: parent.right
        anchors.rightMargin: 4
        anchors.left: parent.left
        anchors.leftMargin: 4
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 4
        anchors.top: parent.top
        anchors.topMargin: 4
    }

    Text {
        id: letter
        width: 50
        text: glyph
        anchors.right: parent.right
        anchors.rightMargin: 0
        anchors.left: parent.left
        anchors.leftMargin: 0
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 0
        anchors.top: parent.top
        anchors.topMargin: 4
        horizontalAlignment: Text.AlignHCenter
        font.pixelSize: 32
    }

    states: [
        State {
            name: "mouseOver"

            PropertyChanges {
                target: roundedRect
                color: "#ffff29"
            }
        },
        State {
            name: "mouseClicked"
            PropertyChanges {
                target: roundedRect
                color: "#e95a52"
            }
        },
        State {
            name: "mouseDepressed"

            PropertyChanges {
                target: roundedRect
                color: "#393939"
            }

            PropertyChanges {
                target: letter
                color: "#fbfbfb"
            }
        }
    ]
}
