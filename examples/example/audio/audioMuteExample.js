//
//  audioMuteExample.js
//  examples
//
//  Created by Thijs Wenker on 10/31/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  This example shows how to use the AudioDevice mute functions.
//  Press the MUTE/UNMUTE button to see it function. 
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

var toggleMuteButton = Overlays.addOverlay("text", {
    x: 50,
    y: 50,
    width: 190,
    height: 50,
    backgroundColor: { red: 255, green: 255, blue: 255},
    color: { red: 255, green: 0, blue: 0},
    font: {size: 30},
    topMargin: 10
});

function muteButtonText() {
    print("Audio Muted: " + AudioDevice.getMuted());
}

function onMuteStateChanged() {
    Overlays.editOverlay(toggleMuteButton,
        AudioDevice.getMuted() ? {text: "UNMUTE", leftMargin: 10} : {text: "MUTE", leftMargin: 38});
    print("Audio Muted: " + AudioDevice.getMuted());
}

function mousePressEvent(event) {
    if (Overlays.getOverlayAtPoint({x: event.x, y: event.y}) == toggleMuteButton) {
        AudioDevice.toggleMute()
    }
}

function scriptEnding() {
    Overlays.deleteOverlay(toggleMuteButton);
}

onMuteStateChanged();

AudioDevice.muteToggled.connect(onMuteStateChanged);
Controller.mousePressEvent.connect(mousePressEvent);
Script.scriptEnding.connect(scriptEnding);