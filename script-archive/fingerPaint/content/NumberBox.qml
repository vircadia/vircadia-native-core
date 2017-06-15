import QtQuick 2.2
import "ColorUtils.js" as ColorUtils

Row {
    id: root
    property string caption: ""
    property real value: 0
    property real min: 0
    property real max: 255
    property int decimals: 2

    QtObject {
        id: m
        // Hack: force update of the text after text validation
        property int forceTextUpdate: 0
    }

    onValueChanged: {

        console.debug("NumberBox root.value:" + root.value)
    }

    width: 60
    height: 20
    spacing: 0
    anchors.margins: 5

    signal accepted(var boxValue)

    Text {
        id: captionBox
        text: root.caption
        width: 18; height: parent.height
        color: "#AAAAAA"
        font.pixelSize: 16; font.bold: true
        horizontalAlignment: Text.AlignRight; verticalAlignment: Text.AlignBottom
        anchors.bottomMargin: 3
    }
    PanelBorder {
        height: parent.height
        anchors.leftMargin: 4;
        anchors.left: captionBox.right; anchors.right: parent.right
        TextInput {
            id: inputBox
            // Hack: force update of the text if the value is the same after the clamp.
            text: m.forceTextUpdate ? root.value.toString() : root.value.toString()
            anchors.leftMargin: 4; anchors.topMargin: 1; anchors.fill: parent
            color: "#AAAAAA"; selectionColor: "#FF7777AA"
            font.pixelSize: 14
            focus: true
            onEditingFinished: {
                var newText = ColorUtils.clamp(parseFloat(inputBox.text), root.min, root.max).toString()
                if(newText != root.value) {
                    root.accepted(newText)
                }
                else {
                    m.forceTextUpdate = m.forceTextUpdate + 1 // Hack: force update
                }
            }
        }
    }
}


