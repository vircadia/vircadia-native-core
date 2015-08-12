import QtQuick 2.4
import QtQuick.Controls 2.3
import QtQuick.Controls.Styles 1.3


Item {
    implicitHeight: 200
    implicitWidth: 800


    TextArea {
        id: gutter
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        style: TextAreaStyle {
            backgroundColor: "grey"
        }
        width: 16
        text: ">"
        font.family: "Lucida Console"
    }
    TextArea {
        anchors.left: gutter.right
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.right: parent.right
        text: "undefined"
        font.family: "Lucida Console"

    }
}

