//
// #20622:  JS Stream Player
// *************************
//
// Created by Kevin M. Thomas and Thoys 07/17/15.
// Copyright 2015 High Fidelity, Inc.
// kevintown.net
//
// JavaScript for the High Fidelity interface that creates a stream player with a UI and keyPressEvents for adding a stream URL in addition to play, stop and volume functionality.
//
// Distributed under the Apache License, Version 2.0.
// See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//


// Declare HiFi public bucket.
HIFI_PUBLIC_BUCKET = "http://s3.amazonaws.com/hifi-public/";

// Declare variables and set up new WebWindow.
var stream;
var volume = 1;
var streamWindow = new WebWindow('Stream', HIFI_PUBLIC_BUCKET + "examples/html/jsstreamplayer.html", 0, 0, false);

// Set up toggleStreamURLButton overlay.
var toggleStreamURLButton = Overlays.addOverlay("text", {
    x: 76,
    y: 275,
    width: 40,
    height: 28,
    backgroundColor: {red: 0, green: 0, blue: 0},
    color: {red: 255, green: 255, blue: 0},
    font: {size: 15},
    topMargin: 8,
    text: " URL"
});

// Set up toggleStreamPlayButton overlay.
var toggleStreamPlayButton = Overlays.addOverlay("text", {
    x: 122,
    y: 275,
    width: 38,
    height: 28,
    backgroundColor: { red: 0, green: 0, blue: 0},
    color: { red: 255, green: 255, blue: 0},
    font: {size: 15},
    topMargin: 8,
    text: " Play"
});

// Set up toggleStreamStopButton overlay.
var toggleStreamStopButton = Overlays.addOverlay("text", {
    x: 166,
    y: 275,
    width: 40,
    height: 28,
    backgroundColor: { red: 0, green: 0, blue: 0},
    color: { red: 255, green: 255, blue: 0},
    font: {size: 15},
    topMargin: 8,
    text: " Stop"
});

// Set up increaseVolumeButton overlay.
var toggleIncreaseVolumeButton = Overlays.addOverlay("text", {
    x: 211,
    y: 275,
    width: 18,
    height: 28,
    backgroundColor: { red: 0, green: 0, blue: 0},
    color: { red: 255, green: 255, blue: 0},
    font: {size: 15},
    topMargin: 8,
    text: " +"
});

// Set up decreaseVolumeButton overlay.
var toggleDecreaseVolumeButton = Overlays.addOverlay("text", {
    x: 234,
    y: 275,
    width: 15,
    height: 28,
    backgroundColor: { red: 0, green: 0, blue: 0},
    color: { red: 255, green: 255, blue: 0},
    font: {size: 15},
    topMargin: 8,
    text: " -"
});

// Function that adds mousePressEvent functionality to connect UI to enter stream URL, play and stop stream.
function mousePressEvent(event) {
    if (Overlays.getOverlayAtPoint({x: event.x, y: event.y}) == toggleStreamURLButton) {
        stream = Window.prompt("Enter Stream: ");
        var streamJSON = {
            action: "changeStream",
            stream: stream
        }
        streamWindow.eventBridge.emitScriptEvent(JSON.stringify(streamJSON));
    }
    if (Overlays.getOverlayAtPoint({x: event.x, y: event.y}) == toggleStreamPlayButton) {
        var streamJSON = {
            action: "changeStream",
            stream: stream
        }
        streamWindow.eventBridge.emitScriptEvent(JSON.stringify(streamJSON));
    }
    if (Overlays.getOverlayAtPoint({x: event.x, y: event.y}) == toggleStreamStopButton) {
        var streamJSON = {
            action: "changeStream",
            stream: ""
        }
        streamWindow.eventBridge.emitScriptEvent(JSON.stringify(streamJSON));
    }
    if (Overlays.getOverlayAtPoint({x: event.x, y: event.y}) == toggleIncreaseVolumeButton) {
        volume += 0.2;
        var volumeJSON = {
            action: "changeVolume",
            volume: volume
        }
        streamWindow.eventBridge.emitScriptEvent(JSON.stringify(volumeJSON));
    }
    if (Overlays.getOverlayAtPoint({x: event.x, y: event.y}) == toggleDecreaseVolumeButton) {
        volume -= 0.2;
        var volumeJSON = {
            action: "changeVolume",
            volume: volume
        }
        streamWindow.eventBridge.emitScriptEvent(JSON.stringify(volumeJSON));
    }
}

// Call function.
Controller.mousePressEvent.connect(mousePressEvent);
streamWindow.setVisible(false);

// Function to delete overlays upon exit.
function onScriptEnding() {
    Overlays.deleteOverlay(toggleStreamURLButton);
    Overlays.deleteOverlay(toggleStreamPlayButton);
    Overlays.deleteOverlay(toggleStreamStopButton);
    Overlays.deleteOverlay(toggleIncreaseVolumeButton);
    Overlays.deleteOverlay(toggleDecreaseVolumeButton);
}

// Call function.
Script.scriptEnding.connect(onScriptEnding);