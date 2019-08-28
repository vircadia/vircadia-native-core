// root.qml

import QtQuick 2.3
import QtQuick.Controls 2.1
import "HFControls"
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

    HFTextField {
        id: textField
        width: 150
        height: 50
        anchors {
            left: root.left
            leftMargin: 40
            top: text.bottom
            topMargin: 20
        }
        echoMode: TextInput.Password
        placeholderText: "Organization"
    }


   HFButton {
        anchors {
            left: root.left
            leftMargin: 40
            top: textField.bottom
            topMargin: 20
        }
        id: button
        text: "NEXT"
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
