import QtQuick 2.3 as Original
import "../styles"
import "."

Original.Item {
    property alias text: label.text
    property alias value: spinBox.value

    Text {
        id: label
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        verticalAlignment: Original.Text.AlignVCenter
        text: "Minimum HMD FPS"
    }
    SpinBox {
        id: spinBox
        width: 120
        maximumValue: 240
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.bottom: parent.bottom
    }

}
