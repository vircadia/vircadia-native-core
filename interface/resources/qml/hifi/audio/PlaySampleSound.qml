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

import QtQuick 2.5
import QtQuick.Controls 1.4
import QtQuick.Controls.Styles 1.4
import QtQuick.Layouts 1.3

import "../../styles-uit"
import "../../controls-uit" as HifiControls

RowLayout {
    property var sound: null;
    property var sample: null;
    property bool isPlaying: false;
    function createSampleSound() {
        var SOUND = Qt.resolvedUrl("../../../sounds/sample.wav");
        sound = SoundCache.getSound(SOUND);
        sample = null;
    }
    function playSound() {
        // FIXME: MyAvatar is not properly exposed to QML; MyAvatar.qmlPosition is a stopgap
        // FIXME: Audio.playSystemSound should not require position
        sample = Audio.playSystemSound(sound, MyAvatar.qmlPosition);
        isPlaying = true;
        sample.finished.connect(function() { isPlaying = false; sample = null; });
    }
    function stopSound() {
        sample && sample.stop();
    }

    Component.onCompleted: createSampleSound();
    Component.onDestruction: stopSound();
    onVisibleChanged: {
        if (!visible) {
            stopSound();
        }
    }

    HifiConstants { id: hifi; }

    Button {
        style: ButtonStyle {
            background: Rectangle {
                implicitWidth: 20;
                implicitHeight: 20;
                radius: hifi.buttons.radius;
                gradient: Gradient {
                    GradientStop {
                        position: 0.2;
                        color: isPlaying ? hifi.buttons.colorStart[hifi.buttons.blue] : hifi.buttons.colorStart[hifi.buttons.black];
                    }
                    GradientStop {
                        position: 1.0;
                        color: isPlaying ? hifi.buttons.colorFinish[hifi.buttons.blue] : hifi.buttons.colorFinish[hifi.buttons.black];
                    }
                }
            }
            label: HiFiGlyphs {
                // absolutely position due to asymmetry in glyph
                x: isPlaying ? 0 : 1;
                y: 1;
                size: 14;
                color: (control.pressed || control.hovered) ? (isPlaying ? "black" : hifi.colors.primaryHighlight) : "white";
                text: isPlaying ? hifi.glyphs.stop_square : hifi.glyphs.playback_play;
            }
        }
        onClicked: isPlaying ? stopSound() : playSound();
    }

    RalewayRegular {
        Layout.leftMargin: 2;
        size: 14;
        color: "white";
        text: isPlaying ? qsTr("Stop sample sound") : qsTr("Play sample sound");
    }

}
