import "../../controls" as HifiControls
import "../../styles-uit"

import QtQuick 2.0
import QtQuick.Controls 2.2

TextField {
    id: control
    font.family: "Fira Sans"
    font.pixelSize: 15;
    color: 'black'

    AvatarAppStyle {
        id: style
    }

    background: Rectangle {
        implicitWidth: 200
        implicitHeight: 40
        color: style.colors.inputFieldBackground
        border.color: style.colors.lightGray
    }
}