import QtQuick 2.3 as Original
import "../styles"
import "."

Original.TextInput {
    id: root
    HifiConstants { id: hifi }
    property string helperText
    height: hifi.layout.rowHeight
    clip: true
    color: hifi.colors.text
    verticalAlignment: Original.TextInput.AlignVCenter
    font.family: hifi.fonts.fontFamily
    font.pointSize: hifi.fonts.fontSize

/*
    Original.Rectangle {
        // Render the rectangle as background
        z: -1
        anchors.fill: parent
        color: hifi.colors.inputBackground
    }
*/
    Text {
        anchors.fill: parent
        font.pointSize: parent.font.pointSize
        font.family: parent.font.family
        verticalAlignment: parent.verticalAlignment
        horizontalAlignment: parent.horizontalAlignment
        text: root.helperText
        color: hifi.colors.hintText
        visible: !root.text
    }
}


