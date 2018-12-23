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
            root.uploader.finished.connect(function() {
                console.log("Did complete");
                backToProjectTimer.start();
            });
        }
    }

    Item {
        id: uploadStatus

        visible: !!root.uploader

        anchors.fill: parent

        Item {
            id: statusItem

            width: parent.width
            height: 128

            AnimatedImage {
                id: uploadSpinner

                visible: !!root.uploader && !root.uploader.complete

                anchors {
                    horizontalCenter: parent.horizontalCenter
                    verticalCenter: parent.verticalCenter
                }

                source: "../../../icons/loader-snake-64-w.gif"
                playing: true
                z: 10000
            }

            HiFiGlyphs {
                id: errorIcon
                visible: !!root.uploader && root.uploader.complete && root.uploader.error !== 0

                anchors {
                    horizontalCenter: parent.horizontalCenter
                    verticalCenter: parent.verticalCenter
                }

                size: 128
                text: "w"
                color: "red"
            }

            HiFiGlyphs {
                id: successIcon

                visible: !!root.uploader && root.uploader.complete && root.uploader.error === 0

                anchors {
                    horizontalCenter: parent.horizontalCenter
                    verticalCenter: parent.verticalCenter
                }

                size: 128
                text: "\ue01a"
                color: "#1FC6A6"
            }
        }
        Item {
            id: statusRows

            anchors.top: statusItem.bottom

            AvatarUploadStatusItem {
                id: statusCategories
                text: "Retreiving categories"

                state: root.uploader.state == 1 ? "running" : (root.uploader.state > 1 ? "success" : (root.uploader.error ? "fail" : ""))
            }
            AvatarUploadStatusItem {
                id: statusUploading
                anchors.top: statusCategories.bottom
                text: "Uploading data"

                state: root.uploader.state == 2 ? "running" : (root.uploader.state > 2 ? "success" : (root.uploader.error ? "fail" : ""))
            }
            // TODO add waiting for response
            //AvatarUploadStatusItem {
                //id: statusResponse
                //text: "Waiting for completion"
            //}
            AvatarUploadStatusItem {
                id: statusInventory
                anchors.top: statusUploading.bottom
                text: "Waiting for inventory"

                state: root.uploader.state == 3 ? "running" : (root.uploader.state > 3 ? "success" : (root.uploader.error ? "fail" : ""))
            }
        }

    }

    Column {
        id: debugInfo

        visible: false

        Text {
            text: "Uploading"
            color: "white"

        }
        Text {
            text: "State: " + (!!root.uploader ? root.uploader.state : " NONE")
            color: "white"
        }
    }

}