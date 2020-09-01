//
// #20485:  AFK - Away From Keyboard Setting
// *****************************************
//
// Created by Kevin M. Thomas and Thoys 07/16/15.
// Copyright 2015 High Fidelity, Inc.
// kevintown.net
//
// JavaScript for the Vircadia interface that creates an away from keyboard functionality by providing a UI and keyPressEvent which will mute toggle the connected microphone, face tracking dde and set the avatar to a hand raise pose.
//
// Distributed under the Apache License, Version 2.0.
// See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//


var originalOutputDevice;
var originalName;
var muted = false;
var wasAudioEnabled;
var afkText = "AFK - I Will Return!\n";

// Set up toggleMuteButton text overlay.
var toggleMuteButton = Overlays.addOverlay("text", {
    x: 10,
    y: 275,
    width: 60,
    height: 28,
    backgroundColor: { red: 0, green: 0, blue: 0},
    color: { red: 255, green: 255, blue: 0},
    font: {size: 15},
    topMargin: 8
});

// Function that overlays text upon state change.
function onMuteStateChanged() {
    Overlays.editOverlay(toggleMuteButton, muted ? {text: "Go Live", leftMargin: 5} : {text: "Go AFK", leftMargin: 5});
}

function toggleMute() {
    if (!muted) {
        if (!AudioDevice.getMuted()) {
            AudioDevice.toggleMute();
        }
        originalOutputDevice = AudioDevice.getOutputDevice();
        Menu.setIsOptionChecked("Mute Face Tracking", true);
        originalName = MyAvatar.displayName;
        AudioDevice.setOutputDevice("none");
        MyAvatar.displayName = afkText + MyAvatar.displayName;
        MyAvatar.setJointData("LeftShoulder", Quat.fromPitchYawRollDegrees(0, 180, 0));
        MyAvatar.setJointData("RightShoulder", Quat.fromPitchYawRollDegrees(0, 180, 0));
    } else {
        if (AudioDevice.getMuted()) {
            AudioDevice.toggleMute();
        }
        AudioDevice.setOutputDevice(originalOutputDevice);
        Menu.setIsOptionChecked("Mute Face Tracking", false);
        MyAvatar.setJointData("LeftShoulder", Quat.fromPitchYawRollDegrees(0, 0, 0));
        MyAvatar.setJointData("RightShoulder", Quat.fromPitchYawRollDegrees(0, 0, 0));
        MyAvatar.clearJointData("LeftShoulder");
        MyAvatar.clearJointData("RightShoulder");
        MyAvatar.displayName = originalName;
    }
    muted = !muted;
    onMuteStateChanged();
}

// Function that adds mousePressEvent functionality to toggle mic mute, AFK message above display name and toggle avatar arms upward.
function mousePressEvent(event) {
    if (Overlays.getOverlayAtPoint({x: event.x, y: event.y}) == toggleMuteButton) {
        toggleMute();
    }
}

// Call functions.
onMuteStateChanged();

//AudioDevice.muteToggled.connect(onMuteStateChanged);
Controller.mousePressEvent.connect(mousePressEvent);

// Function that adds keyPressEvent functionality to toggle mic mute, AFK message above display name and toggle avatar arms upward.
Controller.keyPressEvent.connect(function(event) {
    if (event.text == "y") {
        toggleMute();
    } 
});

// Function that sets a timeout value of 1 second so that the display name does not get overwritten in the event of a crash.
Script.setTimeout(function() { 
    MyAvatar.displayName = MyAvatar.displayName.replace(afkText, "");
}, 1000);

// Function that calls upon exit to restore avatar display name to original state.
Script.scriptEnding.connect(function(){
    if (muted) {
        AudioDevice.setOutputDevice(originalOutputDevice);
        Overlays.deleteOverlay(toggleMuteButton);
        MyAvatar.displayName = originalName;
    }
    Overlays.deleteOverlay(toggleMuteButton);
});