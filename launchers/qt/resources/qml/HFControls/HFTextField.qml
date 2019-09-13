import QtQuick 2.3
import QtQuick.Controls 2.1

TextField {
    id: control
    //color: "#000000"
    font.family: "Graphik Medium"
    font.pixelSize: 22
    verticalAlignment: TextInput.AlignVCenter
    horizontalAlignment: TextInput.AlignLeft
    placeholderText: "PlaceHolder"
    property string seperatorColor: "#FFFFFF"
    selectByMouse: true
    background: Item {
        anchors.fill: parent
        Rectangle {
            color: "#FFFFFF"
            anchors.fill: parent
        }
    }
}
