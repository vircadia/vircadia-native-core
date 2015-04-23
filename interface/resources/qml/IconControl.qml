import QtQuick 2.3
import QtQuick.Controls 1.3
import QtQuick.Window 2.2
import QtQuick.Controls.Styles 1.3

Button {
    text: "Text"
    style: ButtonStyle {
        background:  Item { anchors.fill: parent }
        label: Text {
            id: icon
            width: height
            verticalAlignment: Text.AlignVCenter
            renderType: Text.NativeRendering
            font.family: iconFont.name
            font.pointSize: 18
            property alias unicode: icon.text
            FontLoader { id: iconFont; source: "/fonts/fontawesome-webfont.ttf"; }
            text: control.text
            color: control.enabled ? "white" : "dimgray"
        }
    }
}

