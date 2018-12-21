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
        // TODO Use a shadow instead / border is just here for element debug purposes
        border.width: 2;
    }

}