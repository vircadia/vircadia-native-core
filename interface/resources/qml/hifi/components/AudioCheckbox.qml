import QtQuick 2.5
import QtQuick.Controls 1.4

import "../../styles-uit"
import "../../controls-uit" as HifiControls

Row {
    id: row
    spacing: 16
    property alias checkbox: cb
    property alias cbchecked: cb.checked
    property alias text: txt
    signal checkBoxClicked(bool checked)

    HifiControls.CheckBox {
        id: cb
        boxSize: 20
        colorScheme: hifi.colorSchemes.dark
        anchors.verticalCenter: parent.verticalCenter
        onClicked: checkBoxClicked(cb.checked)
    }
    RalewayBold {
        id: txt
        wrapMode: Text.WordWrap
        width: parent.width - cb.boxSize - 30
        anchors.verticalCenter: parent.verticalCenter
        size: 16
        color: "white"
    }
}
