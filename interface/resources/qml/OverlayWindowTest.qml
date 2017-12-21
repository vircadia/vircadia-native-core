import QtQuick 2.5
import QtQuick.Controls 1.4

Rectangle {
    width: 100
    height: 100
    color: "white"
    Rectangle {
        width: 10
        height: 10
        color: "red"
    }

    Label {
        text: OverlayWindowTestString
        anchors.centerIn: parent
    }
}
