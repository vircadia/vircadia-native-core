import QtQuick 2.6
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3
import QtGraphicalEffects 1.0

import QtQuick.Controls 2.2 as Original

import "../../controlsUit" 1.0 as HifiControls
import "../../stylesUit" 1.0


Item {
    id: uploadingScreen

    property var root: undefined
    //visible: !!root.uploader
    visible: false
    anchors.fill: parent

    Timer {
        id: backToProjectTimer
        interval: 2000
        running: false
        repeat: false
        onTriggered: avatarPackager.state = "project"
    }

    onVisibleChanged: {
        console.log("Visibility changed");
        if (visible) {
            root.uploader.completed.connect(function() {
                console.log("Did complete");
                backToProjectTimer.start();
            });
        }
    }

    Item {
        visible: !!root.uploader && !root.uploader.complete

        anchors.fill: parent

        AnimatedImage {
            id: uploadSpinner

            anchors {
                horizontalCenter: parent.horizontalCenter
                verticalCenter: parent.verticalCenter
            }

            source: "../../../icons/loader-snake-64-w.gif"
            playing: true
            z: 10000
        }
    }

    Item {
        id: failureScreen

        visible: !!root.uploader && root.uploader.complete && root.uploader.error !== 0

        anchors.fill: parent

        HiFiGlyphs {
            id: errorIcon

            anchors {
                horizontalCenter: parent.horizontalCenter
                verticalCenter: parent.verticalCenter
            }

            size: 128
            text: "w"
            color: "red"
        }

        Column {
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.top: errorIcon.bottom
            Text {
                anchors.horizontalCenter: parent.horizontalCenter

                text: "Error"
                font.pointSize: 24

                color: "white"
            }
            Text {
                text: "Your avatar has not been uploaded."
                font.pointSize: 16

                anchors.horizontalCenter: parent.horizontalCenter

                color: "white"
            }
        }
    }

    Item {
        id: successScreen

        visible: !!root.uploader && root.uploader.complete && root.uploader.error === 0

        anchors.fill: parent

        HiFiGlyphs {
            id: successIcon

            anchors {
                horizontalCenter: parent.horizontalCenter
                verticalCenter: parent.verticalCenter
            }

            size: 128
            text: "\ue01a"
            color: "#1FC6A6"
        }

        Column {
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.top: successIcon.bottom

            Text {
                id: successText

                anchors.horizontalCenter: parent.horizontalCenter

                text: "Congratulations!"
                font.pointSize: 24

                color: "white"
            }
            Text {
                text: "Your avatar has been uploaded."
                font.pointSize: 16

                anchors.horizontalCenter: parent.horizontalCenter

                color: "white"
            }
        }

        HifiControls.Button {
            width: implicitWidth
            height: implicitHeight

            anchors.bottom: parent.bottom
            anchors.right: parent.right

            text: "View in Inventory"

            color: hifi.buttons.blue
            colorScheme: root.colorScheme
            onClicked: function() {
                console.log("Opening in inventory");

                AvatarPackagerCore.currentAvatarProject.openInInventory();
            }
        }
    }

    Column {
        id: debugInfo

        visible: false

        Text {
            id: uploadStatus

            text: "Uploading"
            color: "white"

        }
        Text {
            text: "State: " + (!!root.uploader ? root.uploader.state : " NONE")
            color: "white"
        }
    }

}