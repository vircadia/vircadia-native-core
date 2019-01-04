import QtQuick 2.6

import "../../controlsUit" 1.0 as HifiControls
import "../../stylesUit" 1.0

import TabletScriptingInterface 1.0

RalewaySemiBold {
    id: root

    property color idleColor: "white"
    property color hoverColor: "#AFAFAF"
    property color pressedColor: "#575757"

    color: clickable.hovered ? root.hoverColor : (clickable.pressed ? root.pressedColor : root.idleColor)

    signal clicked()

    ClickableArea {
        id: clickable

        anchors.fill: root

        onClicked: root.clicked()
    }
}
