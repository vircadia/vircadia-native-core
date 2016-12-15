import QtQuick 2.0
import QtGraphicalEffects 1.0

Item {
    id: tabletButton
    property string color: "#1080B8"
    property string text: "EDIT"
    property string icon: "icons/edit-icon.svg"
    width: 132
    height: 132

    signal clicked()

    Rectangle {
        id: buttonBg
        color: tabletButton.color
        border.width: 0
        border.color: "#00000000"
        anchors.right: parent.right
        anchors.rightMargin: 0
        anchors.left: parent.left
        anchors.leftMargin: 0
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 0
        anchors.top: parent.top
        anchors.topMargin: 0
    }

    Image {
        id: icon
        width: 60
        height: 60
        anchors.bottom: text.top
        anchors.bottomMargin: 5
        anchors.horizontalCenter: parent.horizontalCenter
        fillMode: Image.Stretch
        source: "../../../" + tabletButton.icon
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

    DropShadow {
        id: dropshadow
        anchors.fill: parent
        horizontalOffset: 0
        verticalOffset: 3
        color: "#aa000000"
        radius: 20
        z: -1
        samples: 41
        source: buttonBg
    }

    MouseArea {
        anchors.fill: parent
        hoverEnabled: true
        onClicked: tabletButton.clicked();
        onEntered: {
            console.log("Tablet Button Hovered!");
            tabletButton.state = "hover state";
        }
        onExited: {
            console.log("Tablet Button Unhovered!");
            tabletButton.state = "base state";
        }
    }

    states: [
        State {
            name: "hover state"

            PropertyChanges {
                target: buttonBg
                border.width: 2
                border.color: "#ffffff"
            }
            PropertyChanges {
                target: dropshadow
                verticalOffset: 0
                color: "#ffffff"
            }
        }
    ]
}


