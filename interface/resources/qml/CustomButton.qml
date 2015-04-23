import QtQuick 2.3
import QtQuick.Controls 1.3
import QtQuick.Window 2.2
import QtQuick.Controls.Styles 1.3

Button {
    SystemPalette { id: myPalette; colorGroup: SystemPalette.Active }
    text: "Text"
    width: 128
    height: 64
    style: ButtonStyle {
        background:  CustomBorder {
            anchors.fill: parent
        }
        label: CustomText {
           renderType: Text.NativeRendering
           verticalAlignment: Text.AlignVCenter
           horizontalAlignment: Text.AlignHCenter
           text: control.text
           color: control.enabled ? myPalette.text : myPalette.dark
        }
    }
}
