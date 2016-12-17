import QtQuick 2.0
import QtGraphicalEffects 1.0

Item {
    id: tabletButton
    property var uuid;
    property string text: "EDIT"
    property string icon: "icons/edit-icon.svg"
    property bool isActive: false
    width: 129
    height: 129

    signal clicked()

    function changeProperty(key, value) {
        tabletButton[key] = value;
    }

    Rectangle {
        id: buttonBg
        color: "#000000"
        opacity: 0.1
        radius: 8
        anchors.right: parent.right
        anchors.rightMargin: 0
        anchors.left: parent.left
        anchors.leftMargin: 0
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 0
        anchors.top: parent.top
        anchors.topMargin: 0
    }

    Rectangle {
        id: buttonOutline
        color: "#00000000"
        opacity: 0.2
        radius: 8
        z: 1
        border.width: 2
        border.color: "#ffffff"
        anchors.right: parent.right
        anchors.rightMargin: 0
        anchors.left: parent.left
        anchors.leftMargin: 0
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 0
        anchors.top: parent.top
        anchors.topMargin: 0
    }

    DropShadow {
            id: glow
            visible: false
            anchors.fill: parent
            horizontalOffset: 0
            verticalOffset: 0
            color: "#ffffff"
            radius: 20
            z: -1
            samples: 41
            source: buttonOutline
    }


    Image {
        id: icon
        width: 50
        height: 50
        visible: false
        anchors.bottom: text.top
        anchors.bottomMargin: 5
        anchors.horizontalCenter: parent.horizontalCenter
        fillMode: Image.Stretch
        source: "../../../" + tabletButton.icon
    }

    ColorOverlay {
        id: iconColorOverlay
        anchors.fill: icon
        source: icon
        color: "#ffffff"
    }

    Text {
        id: text
        color: "#ffffff"
        text: tabletButton.text
        font.bold: true
        font.pixelSize: 18
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 20
        anchors.horizontalCenter: parent.horizontalCenter
        horizontalAlignment: Text.AlignHCenter
    }

    MouseArea {
        anchors.fill: parent
        hoverEnabled: true
        onClicked: {
            console.log("Tablet Button Clicked!");
            if (tabletButton.isActive) {
                tabletButton.state = "base state";
                tabletButton.isActive = false;
            } else {
                tabletButton.state = "active state";
                tabletButton.isActive = true;
            }
            tabletButton.clicked();
        }
        onEntered: {
            console.log("Tablet Button Hovered!");
            if (tabletButton.isActive) {
                tabletButton.state = "hover active state";
            } else {
                tabletButton.state = "hover state";
            }
        }
        onExited: {
            console.log("Tablet Button Unhovered!");
            if (tabletButton.isActive) {
                tabletButton.state = "active state";
            } else {
                tabletButton.state = "base state";
            }
        }
    }

    states: [
        State {
            name: "hover state"

            PropertyChanges {
                target: buttonOutline
                border.color: "#1fc6a6"
                opacity: 1
            }

            PropertyChanges {
                target: glow
                visible: true
            }
        },
        State {
            name: "active state"

            PropertyChanges {
                target: buttonOutline
                border.color: "#1fc6a6"
                opacity: 1
            }

            PropertyChanges {
                target: buttonBg
                color: "#1fc6a6"
                opacity: 1
            }

            PropertyChanges {
                target: text
                color: "#333333"
            }

            PropertyChanges {
                target: iconColorOverlay
                color: "#333333"
            }
        },
        State {
            name: "hover active state"

            PropertyChanges {
                target: glow
                visible: true
            }

            PropertyChanges {
                target: buttonOutline
                border.color: "#ffffff"
                opacity: 1
            }

            PropertyChanges {
                target: buttonBg
                color: "#1fc6a6"
                opacity: 1
            }

            PropertyChanges {
                target: text
                color: "#333333"
            }

            PropertyChanges {
                target: iconColorOverlay
                color: "#333333"
            }

        }
    ]
}


