//
//  Created by Bradley Austin Davis on 2015/11/14
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//


import Hifi 1.0
import QtQuick 2.4
import "controls"
import "styles"

VrDialog {
    id: root
    HifiConstants { id: hifi }

    property real spacing: hifi.layout.spacing
    property real outerSpacing: hifi.layout.spacing * 2

    objectName: "RecorderDialog"

    destroyOnInvisible: false
    destroyOnCloseButton: false

    contentImplicitWidth: recorderDialog.width
    contentImplicitHeight: recorderDialog.height

    RecorderDialog {
        id: recorderDialog
        x: root.clientX; y: root.clientY
        width: 800
        height: 128
        signal play()
        signal rewind()

        onPlay: {
            console.log("Pressed play")
            player.isPlaying = !player.isPlaying
        }

        onRewind: {
            console.log("Pressed rewind")
            player.position = 0
        }

        Row {
            height: 32
            ButtonAwesome {
                id: cmdRecord
                visible: root.showRecordButton
                width: 32; height: 32
                text: "\uf111"
                iconColor: "red"
                onClicked: {
                    console.log("Pressed record")
                    status.text = "Recording";
                }
            }
        }
        Text {
            id: status
            anchors.top: parent.top
            anchors.right: parent.right
            width: 128
            text: "Idle"
        }

        Player {
            id: player
            y: root.clientY + 64
            height: 64
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom



//            onClicked: {
//                if (recordTimer.running) {
//                    recordTimer.stop();
//                }
//                recordTimer.start();
//            }
            Timer {
                id: recordTimer;
                interval: 1000; running: false; repeat: false
                onTriggered: {
                    console.log("Recording: " + MyAvatar.isRecording())
                    MyAvatar.startRecording();
                    console.log("Recording: " + MyAvatar.isRecording())
                }
            }

        }

        Component.onCompleted: {
            player.play.connect(play)
            player.rewind.connect(rewind)

        }
    }
}

