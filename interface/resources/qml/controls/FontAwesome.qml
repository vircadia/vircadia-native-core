import QtQuick 2.3

Text {
    id: root
    property int size: 32
    width: size
    height: size
    font.pixelSize: size
    verticalAlignment: Text.AlignVCenter
    horizontalAlignment: Text.AlignLeft
    font.family: "FontAwesome"
}

