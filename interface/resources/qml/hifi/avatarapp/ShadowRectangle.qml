import stylesUit 1.0
import QtQuick 2.9
import QtGraphicalEffects 1.0

Item {
    property alias color: rectangle.color
    property alias gradient: rectangle.gradient
    property alias border: rectangle.border
    property alias radius: rectangle.radius
    property alias dropShadowRadius: shadow.radius
    property alias dropShadowHorizontalOffset: shadow.horizontalOffset
    property alias dropShadowVerticalOffset: shadow.verticalOffset
    property alias dropShadowOpacity: shadow.opacity

    Rectangle {
        id: rectangle
        width: parent.width
        height: parent.height
    }

    DropShadow {
        id: shadow
        anchors.fill: rectangle
        radius: 6
        horizontalOffset: 0
        verticalOffset: 3
        color: Qt.rgba(0, 0, 0, 0.25)
        source: rectangle
    }
}
