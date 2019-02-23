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
import controlsUit 1.0 as HifiControls

RowLayout {
    property bool audioLoopedBack: AudioScriptingInterface.getServerEcho();
    function startAudioLoopback() {
        if (!audioLoopedBack) {
            audioLoopedBack = true;
            AudioScriptingInterface.setServerEcho(true);
        }
    }
    function stopAudioLoopback () {
        if (audioLoopedBack) {
            audioLoopedBack = false;
            AudioScriptingInterface.setServerEcho(false);
        }
    }

    HifiConstants { id: hifi; }

    Button {
        id: control
        background: Rectangle {
            implicitWidth: 20;
            implicitHeight: 20;
            radius: hifi.buttons.radius;
            gradient: Gradient {
                GradientStop {
                    position: 0.2;
                    color: audioLoopedBack ? hifi.buttons.colorStart[hifi.buttons.blue] : hifi.buttons.colorStart[hifi.buttons.black];
                }
                GradientStop {
                    position: 1.0;
                    color: audioLoopedBack ? hifi.buttons.colorFinish[hifi.buttons.blue] : hifi.buttons.colorFinish[hifi.buttons.black];
                }
            }
        }
        contentItem: HiFiGlyphs {
            size: 14;
            color: (control.pressed || control.hovered) ? (audioLoopedBack ? "black" : hifi.colors.primaryHighlight) : "white";
            text: audioLoopedBack ? hifi.glyphs.stop_square : hifi.glyphs.playback_play;
        }

        onClicked: audioLoopedBack ? stopAudioLoopback() : startAudioLoopback();
    }

    RalewayRegular {
        Layout.leftMargin: 2;
        size: 14;
        color: "white";
        text: audioLoopedBack ? qsTr("Disable Audio Loopback") : qsTr("Enable Audio Loopback");
    }
}
