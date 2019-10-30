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
        source: PathUtils.resourcePath("images/hifi_window@2x.png");
        transformOrigin: Item.Center
        rotation: 0
    }


    Image {
        id: logo
        width: 132
        height: 134
         source: PathUtils.resourcePath("images/HiFi_Voxel.png");

        anchors {
            top: root.top
            topMargin: 144
            horizontalCenter: root.horizontalCenter
        }
    }

    HFTextHeader {
        id: header
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        text: "You're all set!"
        anchors {
            top: logo.bottom
            topMargin: 26
            horizontalCenter: logo.horizontalCenter
        }
    }

    HFTextRegular {
        id: description
        text: "We will see you in world."
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        anchors {
            top: header.bottom
            topMargin: 8
            horizontalCenter: header.horizontalCenter
        }
    }

    Component.onCompleted: {
        root.parent.setBuildInfoState("left");
    }
}
