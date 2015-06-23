import Hifi 1.0 as Hifi
import QtQuick 2.3 as Original
import QtQuick.Layouts 1.1
import "controls"
import "styles"

Hifi.Tooltip {
    id: root
    HifiConstants { id: hifi }
    x: (lastMousePosition.x > surfaceSize.width/2) ? lastMousePosition.x - root.width : lastMousePosition.x
    y: (lastMousePosition.y > surfaceSize.height/2) ? lastMousePosition.y - root.height : lastMousePosition.y
    implicitWidth: border.implicitWidth 
    implicitHeight: border.implicitHeight

    Border {
        id: border
        anchors.fill: parent
        implicitWidth: tooltipBackground.implicitWidth
        //implicitHeight: Math.max(text.implicitHeight, 64)
        implicitHeight: tooltipBackground.implicitHeight

        Original.Image {
            id: tooltipBackground
            source: "../images/tooltip_container.svg"
            width: 323
            height: 423
            anchors.fill: parent

            ColumnLayout {
                spacing: 5

                Text {
                    id: textPlace
                    //anchors.fill: parent
                    anchors.margins: 5
                    font.pixelSize: hifi.fonts.pixelSize / 2
                    text: root.text
                    wrapMode: Original.Text.WordWrap
                }

                Original.Image {
                    id: tooltipPic
                    source: "../images/NoPictureProvided.svg"
                    //anchors.fill: parent
                    anchors.margins: 5
                    verticalAlignment: Original.Image.AlignVCenter
                    //horizontalAlignment: Image.AlignHCenter
                }

                Text {
                    id: textDescription
                    //anchors.fill: parent
                    anchors.margins: 5
                    font.pixelSize: hifi.fonts.pixelSize / 2
                    text: root.text
                    wrapMode: Original.Text.WordWrap
                }

            }
        }
    }
}
