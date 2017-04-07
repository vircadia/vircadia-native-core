import QtQuick 2.5
import QtQuick.Controls 1.4

Item {
    property alias imageURL: image.source
    property alias alpha: image.opacity
    property var subImage;
    property int yOffset: 0
    property int buttonState: 1
    property real size: 50
    width: size; height: size
    property bool pinned: false
    clip: true

    function updateYOffset() {
        //make sure offset not set outside image
        yOffset = (size * buttonState >= image.height) ? image.height - size : size * buttonState
    }
    onButtonStateChanged: updateYOffset();

    Component.onCompleted: {
        if (subImage) {
            if (subImage.y) {
                yOffset = subImage.y;
                return;
            }
        }
        updateYOffset();
    }

    Image {
        id: image
        y: -parent.yOffset;
        width: parent.width
        source: "../../../icons/tablet-icons/empty-toolbar-button.svg"
    }
}

