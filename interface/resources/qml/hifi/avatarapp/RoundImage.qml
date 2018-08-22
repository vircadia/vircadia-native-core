import QtQuick 2.0

Item {

    property alias border: borderRectangle.border
    property alias source: image.source
    property alias fillMode: image.fillMode
    property alias radius: mask.radius
    property alias status: image.status
    property alias progress: image.progress

    Image {
        id: image
        anchors.fill: parent
        anchors.margins: borderRectangle.border.width
    }

    Rectangle {
        id: mask
        anchors.fill: image
    }

    TransparencyMask {
        anchors.fill: image
        source: image
        maskSource: mask
    }

    Rectangle {
        id: borderRectangle
        anchors.fill: parent

        radius: mask.radius
        color: "transparent"
    }
}
