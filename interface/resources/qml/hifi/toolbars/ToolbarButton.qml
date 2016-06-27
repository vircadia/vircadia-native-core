import QtQuick 2.5
import QtQuick.Controls 1.4

Item {
    id: button
    property alias imageURL: image.source
    property alias alpha: button.opacity
    property var subImage;
    property int yOffset: 0
    property int buttonState: 0
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


    onButtonStateChanged: {
        yOffset = size * buttonState
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
        anchors.fill: parent
        onClicked: button.clicked();
    }
}

