import QtQuick 2.3
import QtQuick.Controls 2.1
import "HFControls"

Item {
    id: root
    anchors.centerIn: parent

    Image {
        anchors.centerIn: parent
        width: parent.width
        height: parent.height
        mirror: true
        source: "qrc:/images/hifi_window@2x.png"
        transformOrigin: Item.Center
        rotation: 0
    }


     Image {
        id: logo
        width: 132
        height: 134
        source: "qrc:/images/HiFi_Voxel.png"

        anchors {
            top: root.top
            topMargin: 84
            horizontalCenter: root.horizontalCenter
        }
    }

    Text {
        id: header
        width: 87
        height: 31
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        font.family: "Graphik"
        font.bold: true
        font.pixelSize: 28
        color: "#FFFFFF"
        text: "You're all set!"
        anchors {
            top: logo.bottom
            topMargin: 26
            horizontalCenter: logo.horizontalCenter
        }
    }

    Text {
        id: description
        width: 100
        height: 40
        font.family: "Graphik"
        font.pixelSize: 14
        color: "#FFFFFF"
        text: "We will see you in world."
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        anchors {
            top: header.bottom
            topMargin: 8
            horizontalCenter: header.horizontalCenter
        }
    }

}
