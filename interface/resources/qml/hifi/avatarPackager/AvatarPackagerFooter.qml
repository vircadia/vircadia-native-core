import QtQuick 2.6

import "../../controlsUit" 1.0 as HifiControls
import "../../stylesUit" 1.0

Rectangle {
    id: avatarPackagerFooter

    color: "#404040"
    height: content === defaultContent ? 0 : 74
    visible: content !== defaultContent
    width: parent.width

    property var content: Item { id: defaultContent }

    children: [background, content]

    property var background: Rectangle {
        anchors.fill: parent
        color: "#404040"

        Rectangle {
            id: topBorder1

            anchors.top: parent.top

            color: "#252525"
            height: 1
            width: parent.width
        }
        Rectangle {
            id: topBorder2

            anchors.top: topBorder1.bottom

            color: "#575757"
            height: 1
            width: parent.width
        }
    }
}
