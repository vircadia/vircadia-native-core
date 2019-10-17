import QtQuick 2.3
import QtQuick 2.1

Text {
    id: root

    font.family: "Graphik Regular"
    font.pixelSize: 14

    color: "#C4C4C4"
    linkColor: color

    MouseArea {
        anchors.fill: root
        cursorShape: root.hoveredLink ? Qt.PointingHandCursor : Qt.ArrowCursor
        acceptedButtons: Qt.NoButton
    }
}
