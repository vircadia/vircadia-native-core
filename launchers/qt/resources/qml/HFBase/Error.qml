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
            topMargin: 98
            horizontalCenter: root.horizontalCenter
        }
    }

    HFTextHeader {
        id: header
        text: "Uh oh."

        anchors {
            top: logo.bottom
            topMargin: 26
            horizontalCenter: logo.horizontalCenter
        }
    }

    HFTextRegular {
        id: description

        text: "We seem to have a problem."

        anchors {
            top: header.bottom
            topMargin: 8
            horizontalCenter: header.horizontalCenter
        }
    }

    HFTextRegular {
        text: "Please restart."

        anchors {
            top: description.bottom
            topMargin: 1
            horizontalCenter: header.horizontalCenter
        }
    }


    HFButton {
        id: button
        width: 166

        text: "RESTART"

        anchors {
            top: description.bottom
            topMargin: 60
            horizontalCenter: description.horizontalCenter
        }

        onClicked: {
            LauncherState.restart();
        }
    }

    Component.onCompleted: {
        root.parent.setBuildInfoState("right");
    }

}
