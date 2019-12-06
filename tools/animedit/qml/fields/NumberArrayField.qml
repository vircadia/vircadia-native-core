import QtQuick 2.12
import QtQuick.Window 2.12
import QtQuick.Controls 1.6
import QtQuick.Layouts 1.3
import QtQuick.Dialogs 1.0


Row {
    id: row
    height: 20
    width: 300
    anchors.left: parent.left
    anchors.leftMargin: 0
    anchors.right: parent.right
    anchors.rightMargin: 0

    property string key
    property var value
    property bool optional

    function setValue(newValue) {
        parent.fieldChanged(key, newValue);
    }

    Text {
        id: keyText
        y: 5
        width: 100
        text: key + ":"
        font.pixelSize: 12
        color: optional ? "blue" : "black"
    }

    TextField {
        id: valueTextField
        x: 100
        width: 200

        // first start with a regex for an array of numbers
        //    ^\[\s*(\d+)(\s*,\s*(\d+))*\]$|\[\s*\]
        // then a regex for a floating point number
        //    \d+\.?\d*
        // then substitue the second into the \d+ of the first, yeilding this monstrocity.
        //    ^\[\s*(\d+\.?\d*)(\s*,\s*(\d+\.?\d*))*\]$|\[\s*\]

        //validator: RegExpValidator { regExp: /^\[\s*(\d+\.?\d*)(\s*,\s*(\d+\.?\d*))*\]$|\[\s*\]/ }

        text: JSON.stringify(value)
        onEditingFinished: {
            value = JSON.parse(text);
            setValue(value);
        }
    }
}
