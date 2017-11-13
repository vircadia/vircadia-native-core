import QtQuick 2.3
import QtQuick.Controls 2.2 as Original

import "."
import "../styles"

Original.Button {
    HifiConstants { id: hifi }
    padding {
        top: 8
        left: 12
        right: 12
        bottom: 8
    }
    background:  Border {
        anchors.fill: parent
    }
    contentItem: Text {
        verticalAlignment: Text.AlignVCenter
        horizontalAlignment: Text.AlignHCenter
        text: control.text
        color: control.enabled ? hifi.colors.text : hifi.colors.disabledText
    }
}
