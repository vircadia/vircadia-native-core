import QtQuick 2.6

import "../../controlsUit" 1.0 as HifiControls
import "../../stylesUit" 1.0

import TabletScriptingInterface 1.0

RalewaySemiBold {
    id: root

    anchors.fill: textItem

    property var idleColor: "white"
    property var hoverColor: "#AFAFAF"
    property var pressedColor: "#575757"

    color: clickable.hovered ? root.hoverColor : (clickable.pressed ? root.pressedColor : root.idleColor);

    signal clicked()

    ClickableArea {
        id: clickable

        anchors.fill: root

        onClicked: root.clicked()
    }
}