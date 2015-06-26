import Hifi 1.0 as Hifi
import QtQuick 2.4
import QtQuick.Layouts 1.1
import "controls"
import "styles"

Hifi.Tooltip {
    id: root
    HifiConstants { id: hifi }
    x: lastMousePosition.x + offsetX
    y: lastMousePosition.y + offsetY
    property int offsetX: 0
    property int offsetY: 0
    width: border.width
    height: border.height

    Component.onCompleted: {
        offsetX = (lastMousePosition.x > surfaceSize.width/2) ? -root.width : 0
        offsetY = (lastMousePosition.y > surfaceSize.height/2) ? -root.height : 0
    }

    Rectangle {
        id: border
        color: "#7f000000"
        width: 322
        height: col.height + hifi.layout.spacing * 2

        Column {
            id: col
            x: hifi.layout.spacing
            y: hifi.layout.spacing
            anchors.left: parent.left
            anchors.leftMargin: hifi.layout.spacing
            anchors.right: parent.right
            anchors.rightMargin: hifi.layout.spacing
            spacing: 5

            Text {
                id: textPlace
                color: "white"
                font.underline: true
                anchors.left: parent.left
                anchors.right: parent.right
                font.pixelSize: hifi.fonts.pixelSize / 2
                text: root.title
                wrapMode: Text.WrapAnywhere

                /* Uncomment for debugging to see the extent of the
                Rectangle {
                    anchors.fill: parent
                    color: "#7fff00ff"
                }
                */
            }

            Image {
                id: tooltipPic
                source: "../images/NoPictureProvided.svg"
                anchors.left: parent.left
                anchors.right: parent.right
                verticalAlignment: Image.AlignVCenter
            }

            Text {
                id: textDescription
                color: "white"
                width: border.implicitWidth
                anchors.left: parent.left
                anchors.right: parent.right
                font.pixelSize: hifi.fonts.pixelSize / 2
                text: root.description
                wrapMode: Text.WrapAnywhere
            }
        }
    }
}
