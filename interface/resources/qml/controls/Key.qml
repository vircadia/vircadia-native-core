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
        width: 36
        anchors.fill: parent
        hoverEnabled: true

        onCanceled: {
            if (toggled) {
                keyItem.state = "mouseDepressed"
            } else {
                keyItem.state = ""
            }
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
        width: 30
        color: "#121212"
        radius: 2
        border.color: "#00000000"
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
        y: 6
        width: 50
        color: "#ffffff"
        text: glyph
        style: Text.Normal
        font.family: "Tahoma"
        anchors.right: parent.right
        anchors.rightMargin: 0
        anchors.left: parent.left
        anchors.leftMargin: 0
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 0
        anchors.top: parent.top
        anchors.topMargin: 8
        horizontalAlignment: Text.AlignHCenter
        font.pixelSize: 28
    }

    states: [
        State {
            name: "mouseOver"

            PropertyChanges {
                target: roundedRect
                color: "#121212"
                radius: 3
                border.width: 2
                border.color: "#00b4ef"
            }

            PropertyChanges {
                target: letter
                color: "#00b4ef"
                style: Text.Normal
            }
        },
        State {
            name: "mouseClicked"
            PropertyChanges {
                target: roundedRect
                color: "#1080b8"
                border.width: 2
                border.color: "#00b4ef"
            }

            PropertyChanges {
                target: letter
                color: "#121212"
                styleColor: "#00000000"
                style: Text.Normal
            }
        },
        State {
            name: "mouseDepressed"
            PropertyChanges {
                target: roundedRect
                color: "#0578b1"
                border.width: 0
            }

            PropertyChanges {
                target: letter
                color: "#121212"
                styleColor: "#00000000"
                style: Text.Normal
            }
        }
    ]
}
