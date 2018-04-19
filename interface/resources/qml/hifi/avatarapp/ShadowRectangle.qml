import "../../styles-uit"
import QtQuick 2.9
import QtGraphicalEffects 1.0

Item {
    property alias color: rectangle.color
    property alias border: rectangle.border
    property alias radius: rectangle.radius

    Rectangle {
        id: rectangle
        width: parent.width
        height: parent.height
    }

    DropShadow {
        anchors.fill: rectangle
        radius: 6
        horizontalOffset: 0
        verticalOffset: 3
        color: Qt.rgba(0, 0, 0, 0.25)
        source: rectangle
    }
}
