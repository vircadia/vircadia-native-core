import QtQuick 2.0
import QtGraphicalEffects 1.0

Item {
    id: tabletButton
    property string color: "#1080B8"
    property string text: "EDIT"
    property string icon: "icons/edit-icon.svg"
    width: 132
    height: 132

    Rectangle {
        id: buttonBg
        color: tabletButton.color
        anchors.right: parent.right
        anchors.rightMargin: 0
        anchors.left: parent.left
        anchors.leftMargin: 0
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 0
        anchors.top: parent.top
        anchors.topMargin: 0

        layer.enabled: true
        layer.effect: DropShadow {
            horizontalOffset: 0
            verticalOffset: 3
            color: "#aa000000"
            radius: 20
            samples: 41
        }
    }

    Image {
        id: icon
        width: 60
        height: 60
        anchors.bottom: text.top
        anchors.bottomMargin: 5
        anchors.horizontalCenter: parent.horizontalCenter
        fillMode: Image.Stretch
        source: tabletButton.icon
    }

    Text {
        id: text
        color: "#ffffff"
        text: tabletButton.text
        font.bold: true
        font.pixelSize: 18
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 20
        anchors.horizontalCenter: parent.horizontalCenter
        horizontalAlignment: Text.AlignHCenter
    }
}


