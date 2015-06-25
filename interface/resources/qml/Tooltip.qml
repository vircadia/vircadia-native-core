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

    /*
    Border {
        id: border
        anchors.fill: parent
        implicitWidth: tooltipBackground.implicitWidth
        //implicitHeight: Math.max(text.implicitHeight, 64)
        implicitHeight: tooltipBackground.implicitHeight*/

        Original.Rectangle {
            id: border
            color: "#7f000000"
            implicitWidth: 322
            implicitHeight: col.height + hifi.layout.spacing * 2

            ColumnLayout {
                id: col
                spacing: 5

                Text {
                    id: textPlace
                    color: "#ffffff"
                    implicitWidth: 322
                    //anchors.fill: parent
                    anchors.margins: 5
                    font.pixelSize: hifi.fonts.pixelSize * 1.5
                    text: root.text
                    wrapMode: Original.Text.WrapAnywhere
                }

                Original.Image {
                    id: tooltipPic
                    source: "../images/NoPictureProvided.svg"
                    //anchors.fill: parent
                    anchors.margins: 5
                }

                Text {
                    id: textDescription
                    color: "#ffffff"
                    implicitWidth: 322
                    //anchors.fill: parent
                    anchors.margins: 5
                    font.pixelSize: hifi.fonts.pixelSize
                    text: root.text
                    wrapMode: Original.Text.WrapAnywhere
                }

            }
        }
    //}
}