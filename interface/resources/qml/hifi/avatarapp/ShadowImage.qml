import stylesUit 1.0
import QtQuick 2.9
import QtGraphicalEffects 1.0

Item {
    property alias source: image.source
    property alias dropShadowRadius: shadow.radius
    property alias dropShadowHorizontalOffset: shadow.horizontalOffset
    property alias dropShadowVerticalOffset: shadow.verticalOffset
    property alias radius: image.radius
    property alias border: image.border
    property alias status: image.status
    property alias progress: image.progress
    property alias fillMode: image.fillMode

    RoundImage {
        id: image
        width: parent.width
        height: parent.height
        radius: 6
    }

    DropShadow {
        id: shadow
        anchors.fill: image
        radius: 6
        horizontalOffset: 0
        verticalOffset: 3
        color: Qt.rgba(0, 0, 0, 0.25)
        source: image
    }
}
