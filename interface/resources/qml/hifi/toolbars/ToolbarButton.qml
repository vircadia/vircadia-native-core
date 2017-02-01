import QtQuick 2.5
import QtQuick.Controls 1.4

StateImage {
    id: button
    property bool isActive: false
    property bool isEntered: false

    property int imageOffOut: 1
    property int imageOffIn: 3
    property int imageOnOut: 0
    property int imageOnIn: 2

    signal clicked()

    function changeProperty(key, value) {
        button[key] = value;
    }

    function updateState() {
        if (!button.isEntered && !button.isActive) {
            buttonState = imageOffOut;
        } else if (!button.isEntered && button.isActive) {
            buttonState = imageOnOut;
        } else if (button.isEntered && !button.isActive) {
            buttonState = imageOffIn;
        } else {
            buttonState = imageOnIn;
        }
    }

    onIsActiveChanged: updateState();

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
            button.isEntered = true;
            updateState();
        }
        onExited: {
            button.isEntered = false;
            updateState();
        }
    }
}

