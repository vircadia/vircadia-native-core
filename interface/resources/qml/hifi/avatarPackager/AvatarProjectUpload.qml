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
        interval: 2000
        running: false
        repeat: false
        onTriggered: {
            if (avatarPackager.state === AvatarPackagerState.projectUpload) {
                avatarPackager.state = AvatarPackagerState.project;
            }
        }
    }

    function stateChangedCallback(newState) {
        if (newState >= 4) {
            root.uploader.stateChanged.disconnect(stateChangedCallback);
            backToProjectTimer.start();
        }
    }

    onVisibleChanged: {
        if (visible) {
            root.uploader.stateChanged.connect(stateChangedCallback);
            root.uploader.finishedChanged.connect(function() {
                if (root.uploader.error === 0) {
                    backToProjectTimer.start();
                }
            });
        }
    }

    Item {
        id: uploadStatus

        anchors.fill: parent

        Item {
            id: statusItem

            width: parent.width
            height: 256

            states: [
                State {
                    name: "success"
                    when: root.uploader.state >= 4 && root.uploader.error === 0
                    PropertyChanges { target: uploadSpinner; visible: false }
                    PropertyChanges { target: errorIcon; visible: false }
                    PropertyChanges { target: successIcon; visible: true }
                },
                State {
                    name: "error"
                    when: root.uploader.finished && root.uploader.error !== 0
                    PropertyChanges { target: uploadSpinner; visible: false }
                    PropertyChanges { target: errorIcon; visible: true }
                    PropertyChanges { target: successIcon; visible: false }
                    PropertyChanges { target: errorFooter; visible: true }
                    PropertyChanges { target: errorMessage; visible: true }
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

                size: 315
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
                text: "Retrieving categories"

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
            id: errorMessage

            visible: false

            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: errorFooter.top
            anchors.leftMargin: 16
            anchors.rightMargin: 16
            anchors.bottomMargin: 32

            size: 28
            wrapMode: Text.Wrap
            color: "white"
            text: "We couldn't upload your avatar at this time. Please try again later."
        }

        AvatarPackagerFooter {
            id: errorFooter

            anchors.bottom: parent.bottom
            visible: false

            content: Item {
                anchors.fill: parent
                anchors.rightMargin: 17
                HifiControls.Button {
                    id: backButton

                    anchors.verticalCenter: parent.verticalCenter
                    anchors.right: parent.right

                    text: qsTr("Back")
                    color: hifi.buttons.blue
                    colorScheme: root.colorScheme
                    width: 133
                    height: 40
                    onClicked: {
                        avatarPackager.state = AvatarPackagerState.project;
                    }
                }
            }
        }
    }
}
