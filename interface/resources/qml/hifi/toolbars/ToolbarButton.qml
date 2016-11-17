import QtQuick 2.5
import QtQuick.Controls 1.4

StateImage {
    id: button
    property int hoverState: -1
    property int defaultState: -1

    signal clicked()

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
            if (hoverState >= 0) {
                buttonState = hoverState;
            }
        }
        onExited: {
            if (defaultState >= 0) {
                buttonState = defaultState;
            }
        }
    }
}

