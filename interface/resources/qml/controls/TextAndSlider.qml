import QtQuick 2.3 as Original
import "../styles"
import "."

Original.Item {
    property alias text: label.text
    property alias value: slider.value

    Text {
        id: label
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        verticalAlignment: Original.Text.AlignVCenter
    }

    Slider  {
        id: slider
        width: 120
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.bottom: parent.bottom
    }
}
