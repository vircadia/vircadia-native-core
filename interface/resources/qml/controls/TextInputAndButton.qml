import QtQuick 2.3 as Original
import "../styles"
import "."

Original.Item {
    id: root
    HifiConstants { id: hifi }
    height: hifi.layout.rowHeight
    property string text
    property string helperText
    property string buttonText
    property int buttonWidth: 0
    property alias input: input
    property alias button: button
    signal clicked()

    TextInput {
        id: input
        text: root.text
        helperText: root.helperText
        anchors.left: parent.left
        anchors.right: button.left
        anchors.rightMargin: 8
        anchors.bottom: parent.bottom
        anchors.top: parent.top
    }

    Button {
        id: button
        clip: true
        width: root.buttonWidth ? root.buttonWidth : implicitWidth
        text: root.buttonText
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.top: parent.top
        onClicked: root.clicked()
    }
}


