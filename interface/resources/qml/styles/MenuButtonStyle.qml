import QtQuick 2.4
import QtQuick.Controls.Styles 1.3
import "../controls"
import "."
        
ButtonStyle {
    HifiPalette { id: hifiPalette }
    padding {
        top: 2
        left: 4
        right: 4
        bottom: 2
    }
    background: Item {}
    label: Text {
       renderType: Text.NativeRendering
       verticalAlignment: Text.AlignVCenter
       horizontalAlignment: Text.AlignHCenter
       text: control.text
       color: control.enabled ? "yellow" : "brown"
    }
}
