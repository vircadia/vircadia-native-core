import QtQuick 2.5
import QtQuick.Controls 1.4

Item {
    id: button
    property alias imageURL: image.source
    property alias alpha: image.opacity
    property var subImage;
    property int yOffset: 0
    property int buttonState: 0
    property int hoverState: -1
    property int defaultState: -1
    property var toolbar;
    property real size: 50 // toolbar ? toolbar.buttonSize : 50
    width: size; height: size
    property bool pinned: false
    clip: true

    onButtonStateChanged: {
        yOffset = size * buttonState;
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
            if (hoverState > 0) {
                buttonState = hoverState;
            }
        }
        onExited: {
            if (defaultState > 0) {
                buttonState = defaultState;
            }
        }
    }
}

