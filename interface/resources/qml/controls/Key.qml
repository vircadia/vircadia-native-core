import QtQuick 2.0

Item {
    id: keyItem
    width: 45
    height: 50
    property string glyph: "a"


    MouseArea {
        id: mouseArea1
        anchors.fill: parent
        hoverEnabled: true
        onEntered: {
            keyItem.state = "mouseOver"
        }
        onExited: {
            keyItem.state = ""
        }
        onPressed: {
            keyItem.state = "mouseClicked"
        }
        onReleased: {
            keyItem.state = ""
        }
        onClicked: {
        }
        onCanceled: {
            keyItem.state = ""
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
        }
    ]
}
