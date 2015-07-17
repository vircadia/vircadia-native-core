import Hifi 1.0
import QtQuick 2.3
import QtQuick.Controls 1.2

TextOverlayElement {
    id: root
    Rectangle {
        color: root.backgroundColor
        anchors.fill: parent
        Text {
            x: root.leftMargin
            y: root.topMargin
            id: text
            objectName: "textElement"
            text: root.text
            color: root.textColor
            font.family: root.fontFamily
            font.pixelSize: root.fontSize
            lineHeightMode: Text.FixedHeight
            lineHeight: root.lineHeight
        }
    }
}
