import QtQuick 2.3
import QtQuick.Controls 2.1

TextField {
    id: control
    color: "#FFFFFF"
    font.family: "Graphik Medium"
    font.pixelSize: 22
    verticalAlignment: TextInput.AlignVCenter
    horizontalAlignment: TextInput.AlignLeft
    placeholderText: "PlaceHolder"
    property string seperatorColor: "#FFFFFF"
    background: Item {
        anchors.fill: parent
        Rectangle {
            anchors {
                bottom: parent.bottom
                left: parent.left
                leftMargin: 7
                right: parent.right
            }
            height: 1
            color: control.seperatorColor
        }
    }
}
