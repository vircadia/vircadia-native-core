import QtQuick 2.5
import QtQuick.Controls 1.4

Item {
    id: button
    property alias imageURL: image.source
    property alias alpha: button.opacity
    property var subImage;
    property int yOffset: 0
    property var toolbar;
    property real size: 50 // toolbar ? toolbar.buttonSize : 50
    width: size; height: size
    clip: true

    Component.onCompleted: {
        if (subImage) {
            if (subImage.y) {
                yOffset = subImage.y;
            }
        }
    }

    signal clicked()

    Image {
        id: image
        y: -button.yOffset;
        width: parent.width
    }

    MouseArea {
        anchors.fill: parent
        onClicked: {
            console.log("Clicked on button " + image.source  + " named " + button.objectName)
            button.clicked();
        }
    }
}

