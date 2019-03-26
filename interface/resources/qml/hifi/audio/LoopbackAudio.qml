//
//  LoopbackAudio.qml
//  qml/hifi/audio
//
//  Created by Seth Alves on 2019-2-18
//  Copyright 2019 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

import QtQuick 2.7
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3

import stylesUit 1.0
import controlsUit 1.0 as HifiControlsUit

RowLayout {
    property bool audioLoopedBack: AudioScriptingInterface.getServerEcho();
    function startAudioLoopback() {
        if (!audioLoopedBack) {
            audioLoopedBack = true;
            AudioScriptingInterface.setServerEcho(true);
        }
    }
    function stopAudioLoopback() {
        if (audioLoopedBack) {
            audioLoopedBack = false;
            AudioScriptingInterface.setServerEcho(false);
        }
    }

    HifiConstants { id: hifi; }

    Timer {
        id: loopbackTimer
        interval: 8000;
        running: false;
        repeat: false;
        onTriggered: {
            stopAudioLoopback();
        }
    }

    HifiControlsUit.Button {
        text: audioLoopedBack ? qsTr("STOP TESTING VOICE") : qsTr("TEST YOUR VOICE");
        color: audioLoopedBack ? hifi.buttons.red : hifi.buttons.blue;
        fontSize: 15;
        width: 200;
        height: 32;
        onClicked: {
            if (audioLoopedBack) {
                loopbackTimer.stop();
                stopAudioLoopback();
            } else {
                loopbackTimer.restart();
                startAudioLoopback();
            }
        }
    }

//    RalewayRegular {
//        Layout.leftMargin: 2;
//        size: 14;
//        color: "white";
//        font.italic: true
//        text: audioLoopedBack ? qsTr("Speak in your input") : "";
//    }
}
