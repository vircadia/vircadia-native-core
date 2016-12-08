import QtQuick 2.0
import "../../styles-uit/"

Item {
    id: item1
    width: 480
    height: 720

    Rectangle {
        id: bg
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 0
        anchors.right: parent.right
        anchors.rightMargin: 0
        anchors.left: parent.left
        anchors.leftMargin: 0
        anchors.top: parent.top
        anchors.topMargin: 0
        gradient: Gradient {
            GradientStop {
                position: 0
                color: "#7c7c7c"
            }

            GradientStop {
                position: 1
                color: "#000000"
            }
        }

        Flow {
            id: flow
            spacing: 12
            anchors.right: parent.right
            anchors.rightMargin: 17
            anchors.left: parent.left
            anchors.leftMargin: 17
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 8
            anchors.top: parent.top
            anchors.topMargin: 123
        }

        Image {
            id: muteIcon
            x: 205
            width: 70
            height: 70
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.top: parent.top
            anchors.topMargin: 28
            fillMode: Image.Stretch
            source: "../../../icons/circle.svg"

            RalewaySemiBold {
                id: muteText
                x: 15
                y: 27
                color: "#ffffff"
                text: qsTr("MUTE")
                horizontalAlignment: Text.AlignHCenter
                font.pixelSize: 14
            }
        }

        Component.onCompleted: {
            console.log("AJT: Tablet.onCompleted!");
            // AJT: test flow by adding buttons
            var component = Qt.createComponent("TabletButton.qml");
            for (var i = 0; i < 10; i++) {
                component.createObject(flow);
            }
        }
    }
}

