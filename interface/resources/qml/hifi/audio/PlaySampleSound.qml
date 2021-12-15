//
//  PlaySampleSound.qml
//  qml/hifi/audio
//
//  Created by Zach Pomerantz on 6/13/2017
//  Copyright 2017 High Fidelity, Inc.
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
    property var sound: null;
    property var sample: null;
    property bool isPlaying: false;
    function createSampleSound() {
        sound = ApplicationInterface.getSampleSound();
        sample = null;
    }
    function playSound() {
        if (sample === null && !isPlaying) {
            sample = AudioScriptingInterface.playSystemSound(sound);
            isPlaying = true;
            sample.finished.connect(reset);
        }
    }
    function stopSound() {
        if (sample && isPlaying) {
            sample.stop();
        }
    }

    function reset() {
        sample.finished.disconnect(reset);
        isPlaying = false;
        sample = null;
    }

    Component.onCompleted: createSampleSound();
    Component.onDestruction: stopSound();
    onVisibleChanged: {
        if (!visible) {
            stopSound();
        }
    }

    HifiConstants { id: hifi; }

    HifiControlsUit.Button {
        text: isPlaying ? qsTr("STOP TESTING") : qsTr("TEST YOUR SOUND");
        color: isPlaying ? hifi.buttons.red : hifi.buttons.blue;
        onClicked: isPlaying ? stopSound() : playSound();
        fontSize: 15;
        width: 200;
        height: 32;
    }

    RalewayRegular {
        Layout.leftMargin: 2;
        size: 18;
        color: "white";
        font.italic: true
        text: isPlaying ? qsTr("Listen to your output") : "";
    }
}
