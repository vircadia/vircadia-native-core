import QtQuick 2.3
import QtQuick.Controls 1.3 as Original
import QtQuick.Controls.Styles 1.3 as OriginalStyles
import "."
import "../styles"

Original.Button {
    property color iconColor: "black"
    FontLoader { id: iconFont; source: "../../fonts/fontawesome-webfont.ttf"; }
    style: OriginalStyles.ButtonStyle { 
        label: Text {
            renderType: Text.NativeRendering
            verticalAlignment: Text.AlignVCenter
            horizontalAlignment: Text.AlignHCenter
            font.family: iconFont.name
            font.pointSize: 20
            color: control.enabled ? control.iconColor : "gray"
            text: control.text
          }
    }
}
