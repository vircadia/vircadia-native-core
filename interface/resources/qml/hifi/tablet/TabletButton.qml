import QtQuick 2.0

Rectangle {
    id: tabletButton
    width: 132
    height: 132
    color: "#1d94c3"

    Image {
        id: icon
        width: 60
        height: 60
        fillMode: Image.Stretch
        anchors.bottom: text.top
        anchors.bottomMargin: 3
        anchors.horizontalCenterOffset: 0
        anchors.horizontalCenter: parent.horizontalCenter
        source: "../../../icons/edit-icon.svg"
    }

    Text {
        id: text
        color: "#ffffff"
        text: "EDIT"
        font.bold: true
        font.pixelSize: 18
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 20
        anchors.horizontalCenter: parent.horizontalCenter
        horizontalAlignment: Text.AlignHCenter
    }
}
