// root.qml

import QtQuick 2.3
import QtQuick.Controls 2.1
Image {
    id: root
    width: 515
    height: 450
    source: "../images/hifi_window@2x.png"

    Text {
        id: text
        width: 325
        height: 26
        anchors {
            left: root.left
            leftMargin: 95
            top: root.top
            topMargin: 29
        }
        text: "Please log in"
        font.pointSize: 24
        color: "#FFFFFF"
    }

    TextField {
        id: textField
        background: Rectangle {
            color: "#00000000"
        }
        anchors {
            left: root.left
            leftMargin: 40
            top: text.bottom
            topMargin: 20
        }
        echoMode: TextInput.Password
        placeholderText: "Organization"
    }

    Rectangle {
        id: seperator
        anchors {
            left: textField.left
            right: textField.right
            top: textField.bottom
            topMargin: 1
        }

        height: 1
        color: "#FFFFFF"
    }


    Button {
        anchors {
            left: root.left
            leftMargin: 40
            top: textField.bottom
            topMargin: 20
        }
        id: button
        text: "NEXT"
        background: Rectangle {
            implicitWidth: 100
            implicitHeight: 40
            color: "#00000000"
            opacity: 1
            border.color: "#FFFFFF"
            border.width: 4
            radius: 2
        }
    }


    Image {
        id: spinner
        source: "../images/HiFi_Voxel.png"
        anchors.bottom: root.bottom
        RotationAnimator {
            target: spinner;
            loops: Animation.Infinite
            from: 0;
            to: 360;
            duration: 5000
            running: true
        }
    }
}
