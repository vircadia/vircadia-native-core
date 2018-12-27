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
    visible: false
    anchors.fill: parent

    Timer {
        id: backToProjectTimer
        interval: 5000
        running: false
        repeat: false
        onTriggered: {
            if (avatarPackager.state =="project-upload") {
                avatarPackager.state = "project"
            }
        }
    }

    function stateChangedCallback(newState) {
        if (newState >= 4) {
            root.uploader.stateChanged.disconnect(stateChangedCallback)
            backToProjectTimer.start();
        }
    }
    onVisibleChanged: {
        if (visible) {
            root.uploader.stateChanged.connect(stateChangedCallback);
            root.uploader.finishedChanged.connect(function() {
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
            height: 192

            states: [
                State {
                    name: "success"
                    when: !!root.uploader && root.uploader.state >= 4 && root.uploader.error === 0
                    PropertyChanges { target: uploadSpinner; visible: false }
                    PropertyChanges { target: errorIcon; visible: false }
                    PropertyChanges { target: successIcon; visible: true }
                },
                State {
                    name: "error"
                    when: !!root.uploader && root.uploader.finished && root.uploader.error !== 0
                    PropertyChanges { target: uploadSpinner; visible: false }
                    PropertyChanges { target: errorIcon; visible: true }
                    PropertyChanges { target: successIcon; visible: false }
                }
            ]

            AnimatedImage {
                id: uploadSpinner

                visible: true

                anchors {
                    horizontalCenter: parent.horizontalCenter
                    verticalCenter: parent.verticalCenter
                }

                width: 164
                height: 164

                source: "../../../icons/loader-snake-256.gif"
                playing: true
            }

            HiFiGlyphs {
                id: errorIcon

                visible: false

                anchors {
                    horizontalCenter: parent.horizontalCenter
                    verticalCenter: parent.verticalCenter
                }

                size: 164
                text: "+"
                color: "#EA4C5F"
            }

            Image {
                id: successIcon

                visible: false

                anchors.verticalCenter: parent.verticalCenter
                anchors.horizontalCenter: parent.horizontalCenter

                width: 148
                height: 148

                source: "../../../icons/checkmark-stroke.svg"
            }
        }
        Item {
            id: statusRows

            anchors.top: statusItem.bottom
            anchors.left: parent.left
            anchors.leftMargin: 12

            AvatarUploadStatusItem {
                id: statusCategories
                uploader: root.uploader
                text: "Retreiving categories"

                uploaderState: 1
            }
            AvatarUploadStatusItem {
                id: statusUploading
                uploader: root.uploader
                anchors.top: statusCategories.bottom
                text: "Uploading data"

                uploaderState: 2
            }
            AvatarUploadStatusItem {
                id: statusResponse
                uploader: root.uploader
                anchors.top: statusUploading.bottom
                text: "Waiting for response"

                uploaderState: 3
            }
        }

        RalewayRegular {
            visible: root.uploader.error

            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            anchors.leftMargin: 16
            anchors.rightMargin: 16
            anchors.bottomMargin: 16

            size: 28
            wrapMode: Text.Wrap
            color: "white"
            text: "We couldn't upload your avatar at this time. Please try again later."
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
