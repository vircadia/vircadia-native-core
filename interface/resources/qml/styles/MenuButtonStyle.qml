import "../controls"
import "."
		
ButtonStyle {
    SystemPalette { id: myPalette; colorGroup: SystemPalette.Active }
	padding {
        top: 8
        left: 12
        right: 12
        bottom: 8
	}
    background:  Border {
        anchors.fill: parent
        color: "#00000000"
    	borderColor: "red"
    }
    label: Text {
       renderType: Text.NativeRendering
       verticalAlignment: Text.AlignVCenter
       horizontalAlignment: Text.AlignHCenter
       text: control.text
       color: control.enabled ? myPalette.text : myPalette.dark
    }
}
