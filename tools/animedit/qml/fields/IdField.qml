import QtQuick 2.12
import QtQuick.Window 2.12
import QtQuick.Controls 1.6
import QtQuick.Layouts 1.3
import QtQuick.Dialogs 1.0

Row {
    id: row
    x: 0
    y: 0
    height: 20
    anchors.left: parent.left
    anchors.leftMargin: 0
    anchors.right: parent.right
    anchors.rightMargin: 0

    property string theValue

    function setValue(newValue) {
        var ROLE_NAME = 0x0101;
        theModel.setData(leftHandPane.currentIndex, newValue, ROLE_NAME);
    }

    Text {
        id: element
        y: 5
        width: 100
        text: qsTr("Id:")
        font.pixelSize: 12
    }

    TextField {
        id: textField
        x: 100
        width: 200
        text: theValue
        onEditingFinished: {
            setValue(text);
        }
    }
}
