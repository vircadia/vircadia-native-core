import QtQuick 2.5
import QtQuick.Controls 1.4

Item {
    id: button
    property alias imageURL: image.source
    property alias alpha: image.opacity
    property var subImage;
    property int yOffset: 0
    property int buttonState: 0
    property int hoverOffset: 0
    property var toolbar;
    property real size: 50 // toolbar ? toolbar.buttonSize : 50
    width: size; height: size
    property bool pinned: false
    clip: true
    
    function updateOffset() {
        yOffset = size * (buttonState + hoverOffset);
    }
    
    onButtonStateChanged: {
        hoverOffset = 0; // subtle: show the new state without hover. don't wait for mouse to be moved away
        // The above is per UX design, but ALSO avoid a subtle issue that would be a problem because
        // the hand controllers don't move the mouse when not triggered, so releasing the trigger would
        // never show unhovered.
        updateOffset();
    }

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

    Timer {
        id: asyncClickSender
        interval: 10
        repeat: false
        running: false
        onTriggered: button.clicked();
    }
    
    MouseArea {
        id: mouseArea
        hoverEnabled: true
        anchors.fill: parent
        onClicked: asyncClickSender.start();
        onEntered: {
            hoverOffset = 2;
            updateOffset();
        }
        onExited: {
            hoverOffset = 0;
            updateOffset();
        }
    }
}

