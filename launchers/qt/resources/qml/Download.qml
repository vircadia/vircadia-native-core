import QtQuick 2.3
import QtQuick.Controls 2.1
import "HFControls"

Item {
    id: root
    anchors.fill: parent


    Image {
        anchors.centerIn: parent
        width: parent.width
        height: parent.height
        mirror: true
        source: PathUtils.resourcePath("images/hifi_window@2x.png");
        transformOrigin: Item.Center
        rotation: 180
    }

    Image {
        id: logo
        width: 150
        height: 150
        source: PathUtils.resourcePath("images/HiFi_Voxel.png");

        anchors {
            top: root.top
            topMargin: 98
            horizontalCenter: root.horizontalCenter
        }

         RotationAnimator {
            target: logo;
            loops: Animation.Infinite
            from: 0;
            to: 360;
            duration: 5000
            running: true
        }
    }

    HFTextHeader {
        id: firstText

        text: "Setup will take a moment"

        anchors {
            top: logo.bottom
            topMargin: 46
            horizontalCenter: logo.horizontalCenter
        }
    }

    HFTextRegular {
        id: secondText

        text: "We're getting everything set up for you."

        anchors {
            top: firstText.bottom
            topMargin: 8
            horizontalCenter: logo.horizontalCenter
        }
    }

    ProgressBar {
        id: progressBar
        width: 394
        height: 8

        value: LauncherState.downloadProgress;

        anchors {
            top: secondText.bottom
            topMargin: 30
            horizontalCenter: logo.horizontalCenter
        }

        background: Rectangle {
            implicitWidth: progressBar.width
            implicitHeight: progressBar.height
            radius: 8
            color: "#252525"
        }

        contentItem: Item {
            implicitWidth: progressBar.width
            implicitHeight: progressBar.height * 0.85

            Rectangle {
                width: progressBar.visualPosition * parent.width
                height: parent.height
                radius: 6
                color: "#01B2ED"
            }
        }
    }

    Component.onCompleted: {
        root.parent.setBuildInfoState("left");
    }
}
