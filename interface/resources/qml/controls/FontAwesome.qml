import QtQuick 2.3

Text {
    id: root
    FontLoader { id: iconFont; source: "qrc:/fonts/fontawesome-webfont.ttf"; }
    property int size: 32
    width: size
    height: size
    font.pixelSize: size
    verticalAlignment: Text.AlignVCenter
    horizontalAlignment: Text.AlignLeft
    font.family: iconFont.name
}

