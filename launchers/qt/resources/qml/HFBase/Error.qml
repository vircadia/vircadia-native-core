import QtQuick 2.3
import QtQuick 2.1
import "../HFControls"


Item {
    id: root
    anchors.centerIn: parent

    Image {
        anchors.centerIn: parent
        width: parent.width
        height: parent.height
        mirror: false
        source: "qrc:/images/hifi_window@2x.png"
        //fillMode: Image.PreserveAspectFit
        transformOrigin: Item.Center
        //rotation: 90
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
        font.family: "Graphik"
        font.bold: true
        font.pixelSize: 28
        color: "#FFFFFF"
        text: "Uh oh."
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
        text: "We seem to have a problem.\n Please restart HQ Launcher"
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        anchors {
            top: header.bottom
            topMargin: 8
            horizontalCenter: header.horizontalCenter
        }
    }


    HFButton {
        id: button
        width: 166
        height: 35
        font.family: "Graphik"
        font.pixelSize: 18
        text: "Restart"

        anchors {
            top: description.bottom
            topMargin: 15
            horizontalCenter: description.horizontalCenter
        }

        onClicked: {
            LauncherState.restart();
        }
    }


    Text {
        id: hifilogo
        width: 100
        height: 18

        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        font.family: "Graphik"
        font.pixelSize: 18
        font.bold: true
        color: "#FFFFFF"

        text: "High Fidelity"
        anchors {
            right: root.right
            rightMargin: 17
            bottom: root.bottom
            bottomMargin: 17
        }
    }

}
