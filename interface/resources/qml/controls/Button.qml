import QtQuick 2.3
import QtQuick.Controls 1.3 as Original
import QtQuick.Controls.Styles 1.3

import "."
import "../styles"

Original.Button {
    style: ButtonStyle {
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
        label: Text {
           verticalAlignment: Text.AlignVCenter
           horizontalAlignment: Text.AlignHCenter
           text: control.text
           color: control.enabled ? hifi.colors.text : hifi.colors.disabledText
        }
    }
}
