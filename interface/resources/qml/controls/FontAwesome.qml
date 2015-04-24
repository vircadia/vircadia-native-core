import QtQuick 2.3
import QtQuick.Controls 1.3
import QtQuick.Controls.Styles 1.3

Text {
    id: root
    FontLoader { id: iconFont; source: "../../fonts/fontawesome-webfont.ttf"; }
    property int size: 32
    width: size
	height: size
    verticalAlignment: Text.AlignVCenter
    horizontalAlignment: Text.AlignHCenter
    font.family: iconFont.name
    font.pointSize: 18
}

