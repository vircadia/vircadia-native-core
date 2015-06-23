import Hifi 1.0 as Hifi
import QtQuick 2.3 as Original
import "controls"
import "styles"

Hifi.Tooltip {
    id: root
    HifiConstants { id: hifi }
    x: (lastMousePosition.x > surfaceSize.width/2) ? lastMousePosition.x - root.width : lastMousePosition.x + 20
    y: (lastMousePosition.y > surfaceSize.height/2) ? lastMousePosition.y - root.height : lastMousePosition.y + 5
    implicitWidth: border.implicitWidth 
    implicitHeight: border.implicitHeight

    Border {
        id: border
        anchors.fill: parent
        implicitWidth: tooltipBackground.implicitWidth
        //implicitHeight: Math.max(text.implicitHeight, 64)
        implicitHeight: tooltipBackground.implicitHeight

        /*
        Text {
            id: text
            anchors.fill: parent
            anchors.margins: 16
            font.pixelSize: hifi.fonts.pixelSize / 2
            text: root.text
            wrapMode: Original.Text.WordWrap
        }*/

        Image {
            id: tooltipBackground
            source: "../images/NoPictureProvided.svg"
            width: 323
            height: 423
            anchors.fill: parent

            /*
            Image {
                id: tooltipBackground
                source: "../images/NoPictureProvided.svg"
                anchors.fill: parent
            }*/
        }
    }
}
