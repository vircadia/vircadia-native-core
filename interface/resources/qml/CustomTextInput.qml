import QtQuick 2.3
import QtQuick.Controls 1.2

TextInput {
    SystemPalette { id: myPalette; colorGroup: SystemPalette.Active }
    property string helperText: ""
    font.family: "Helvetica"
    font.pointSize: 18
    width: 256
    height: 64
    color: myPalette.text
    clip: true
    verticalAlignment: TextInput.AlignVCenter

    onTextChanged: {
        if (text == "") {
            helperText.visible = true;
        } else {
            helperText.visible = false;
        }
    }

    Text {
        id: helperText
        anchors.fill: parent
        font.pointSize: parent.font.pointSize
        font.family: "Helvetica"
        verticalAlignment: TextInput.AlignVCenter
        text: parent.helperText
        color: myPalette.dark
        clip: true
    }
}

