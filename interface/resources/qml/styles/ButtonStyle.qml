import QtQuick 2.4 as Original
import QtQuick.Controls.Styles 1.3 as OriginalStyles
import "."
import "../controls"
        
OriginalStyles.ButtonStyle {
    Original.SystemPalette { id: myPalette; colorGroup: Original.SystemPalette.Active }
    padding {
        top: 8
        left: 12
        right: 12
        bottom: 8
    }
    background:  Border {
        anchors.fill: parent
    }
    label: Text {
       renderType: Original.Text.NativeRendering
       verticalAlignment: Original.Text.AlignVCenter
       horizontalAlignment: Original.Text.AlignHCenter
       text: control.text
       color: control.enabled ? myPalette.text : myPalette.dark
    }
}
