import Hifi 1.0 as Hifi
import QtQuick 2.3 as Original
import "controls"
import "styles"

Hifi.Tooltip {
    id: root
    HifiConstants { id: hifi }
    // FIXME adjust position based on the edges of the screen
    x: lastMousePosition.x + 20
    y: lastMousePosition.y + 5
    implicitWidth: border.implicitWidth 
    implicitHeight: border.implicitHeight

    Border {
        id: border
        anchors.fill: parent
        implicitWidth: text.implicitWidth
        implicitHeight: Math.max(text.implicitHeight, 64)

        Text {
            id: text
            anchors.fill: parent
            anchors.margins: 16
            font.pixelSize: hifi.fonts.pixelSize / 2
            text: root.text
            wrapMode: Original.Text.WordWrap
        }
    }
}
