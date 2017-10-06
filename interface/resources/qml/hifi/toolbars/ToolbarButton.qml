import QtQuick 2.5
import QtQuick.Controls 1.4

StateImage {
    id: button

    property string captionColorOverride: ""
    property bool buttonEnabled: true
    property bool isActive: false
    property bool isEntered: false

    property int imageOffOut: 1
    property int imageOffIn: 3
    property int imageOnOut: 0
    property int imageOnIn: 2

    property string text: ""
    property string hoverText: button.text
    property string activeText: button.text
    property string activeHoverText: button.activeText

    property string icon: "icons/tablet-icons/blank.svg"
    property string hoverIcon: button.icon
    property string activeIcon: button.icon
    property string activeHoverIcon: button.activeIcon

    property int sortOrder: 100
    property int stableSortOrder: 0

    signal clicked()

    function changeProperty(key, value) {
        button[key] = value;
    }

    function urlHelper(src) {
        if (src.match(/\bhttp/)) {
            return src;
        } else {
            return "../../../" + src;
        }
    }

    function updateState() {
        if (buttonEnabled) {
            if (!button.isEntered && !button.isActive) {
                buttonState = imageOffOut;
            } else if (!button.isEntered && button.isActive) {
                buttonState = imageOnOut;
            } else if (button.isEntered && !button.isActive) {
                buttonState = imageOffIn;
            } else {
                buttonState = imageOnIn;
            }
        } else {
            buttonState = 0;
        }
    }

    onIsActiveChanged: updateState();
    onButtonEnabledChanged: updateState();

    Timer {
        id: asyncClickSender
        interval: 10
        repeat: false
        running: false
        onTriggered: {
            button.clicked();
        }
    }

    MouseArea {
        id: mouseArea
        hoverEnabled: buttonEnabled
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

    Image {
        id: icon
        width: 28
        height: 28
        anchors.bottom: caption.top
        anchors.bottomMargin: 0
        anchors.horizontalCenter: parent.horizontalCenter
        fillMode: Image.Stretch
        source: urlHelper(button.isActive ? (button.isEntered ? button.activeHoverIcon : button.activeIcon) : (button.isEntered ? button.hoverIcon : button.icon))
    }

    Text {
        id: caption
        color: captionColorOverride !== "" ? captionColorOverride: (button.isActive ? "#000000" : "#ffffff")
        text: button.isActive ? (button.isEntered ? button.activeHoverText : button.activeText) : (button.isEntered ? button.hoverText : button.text)
        font.bold: false
        font.pixelSize: 9
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 5
        anchors.horizontalCenter: parent.horizontalCenter
        horizontalAlignment: Text.AlignHCenter
    }
}

