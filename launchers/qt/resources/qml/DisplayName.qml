// login

import QtQuick 2.3
import QtQuick 2.1
import "HFControls"

Item {
    id: root
    anchors.fill: parent
    Text {
        id: title
        width: 325
        height: 26
        font.family: "Graphik"
        font.pixelSize: 28
        color: "#FFFFFF"
        text: "Choose a display name"
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        anchors {
            top: root.top
            topMargin: 29
            horizontalCenter: root.horizontalCenter
        }
    }

    Text {
        id: instruction
        width: 425
        height: 22
        font.family: "Graphik"
        font.pixelSize: 14
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        color: "#C4C4C4"
        text: "This is the name that your teammates will see."
        anchors {
            left: root.left
            right: root.right
            top: title.bottom
            topMargin: 18
        }
    }

    HFTextField {
        id: password
        width: 306
        height: 40
        font.family: "Graphik"
        font.pixelSize: 18
        placeholderText: "Display Name"
        color: "#808080"
        seperatorColor: Qt.rgba(1, 1, 1, 0.3)
        anchors {
            top: instruction.bottom
            horizontalCenter: instruction.horizontalCenter
            topMargin: 79
        }
    }

    HFButton {
        id: button
        width: 122
        height: 36

        font.family: "Graphik"
        font.pixelSize: 18
        text: "NEXT"

        anchors {
            top: password.bottom
            horizontalCenter: instruction.horizontalCenter
            topMargin: 59
        }
    }
}
