import QtQuick 2.5
import QtQuick.Controls 1.4

Item {
    id: button
    property alias imageURL: image.source
    property alias alpha: button.opacity
    property var subImage;
    property int yOffset: 0
    property int buttonState: 0
    property int hoverOffset: 0
    property var toolbar;
    property real size: 50 // toolbar ? toolbar.buttonSize : 50
    width: size; height: size
    property bool pinned: false
    clip: true

    Behavior on opacity {
        NumberAnimation {
            duration: 150
            easing.type: Easing.InOutCubic
        }
    }

    property alias fadeTargetProperty: button.opacity

    onFadeTargetPropertyChanged: {
        visible = (fadeTargetProperty !== 0.0);
    }

    onVisibleChanged: {
        if ((!visible && fadeTargetProperty != 0.0) || (visible && fadeTargetProperty == 0.0)) {
            var target = visible;
            visible = !visible;
            fadeTargetProperty = target ? 1.0 : 0.0;
            return;
        }
    }

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

    MouseArea {
        id: mouseArea
        hoverEnabled: true
        anchors.fill: parent
        onClicked: button.clicked();
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

