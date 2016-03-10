//
// #20628:  JS Stream Player Domain-Zone
// *************************************
//
// Created by Kevin M. Thomas, Thoys and Konstantin 07/24/15.
// Copyright 2015 High Fidelity, Inc.
// kevintown.net
//
// JavaScript for the High Fidelity interface that creates a stream player with a UI for playing a domain-zone specificed stream URL in addition to play, stop and volume functionality which is resident only in the domain-zone.
//
// Distributed under the Apache License, Version 2.0.
// See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//


// Declare variables and set up new WebWindow.
var lastZone = "";
var volume = 0.5;
var stream = "";
var streamWindow = new WebWindow('Stream', "https://dl.dropboxusercontent.com/u/17344741/jsstreamplayer/jsstreamplayerdomain-zone.html", 0, 0, false);
var visible = false;

// Set up toggleStreamPlayButton overlay.
var toggleStreamPlayButton = Overlays.addOverlay("text", {
    x: 122,
    y: 310,
    width: 38,
    height: 28,
    backgroundColor: { red: 0, green: 0, blue: 0},
    color: { red: 255, green: 255, blue: 0},
    font: {size: 15},
    topMargin: 8,
    visible: false,
    text: " Play"
});

// Set up toggleStreamStopButton overlay.
var toggleStreamStopButton = Overlays.addOverlay("text", {
    x: 166,
    y: 310,
    width: 40,
    height: 28,
    backgroundColor: { red: 0, green: 0, blue: 0},
    color: { red: 255, green: 255, blue: 0},
    font: {size: 15},
    topMargin: 8,
    visible: false,
    text: " Stop"
});

// Set up increaseVolumeButton overlay.
var toggleIncreaseVolumeButton = Overlays.addOverlay("text", {
    x: 211,
    y: 310,
    width: 18,
    height: 28,
    backgroundColor: { red: 0, green: 0, blue: 0},
    color: { red: 255, green: 255, blue: 0},
    font: {size: 15},
    topMargin: 8,
    visible: false,
    text: " +"
});

// Set up decreaseVolumeButton overlay.
var toggleDecreaseVolumeButton = Overlays.addOverlay("text", {
    x: 234,
    y: 310,
    width: 15,
    height: 28,
    backgroundColor: { red: 0, green: 0, blue: 0},
    color: { red: 255, green: 255, blue: 0},
    font: {size: 15},
    topMargin: 8,
    visible: false,
    text: " -"
});

// Function to change JSON object stream.
function changeStream(stream) {
    var streamJSON = {
        action: "changeStream", 
        stream: stream
    }
    streamWindow.eventBridge.emitScriptEvent(JSON.stringify(streamJSON));
}

// Function to change JSON object volume.
function changeVolume(volume) {
    var volumeJSON = {
            action: "changeVolume",
            volume: volume
        }
        streamWindow.eventBridge.emitScriptEvent(JSON.stringify(volumeJSON));
}

// Function that adds mousePressEvent functionality to connect UI to enter stream URL, play and stop stream.
function mousePressEvent(event) {
    if (Overlays.getOverlayAtPoint({x: event.x, y: event.y}) == toggleStreamPlayButton) {
        changeStream(stream);
        volume = 0.25;
        changeVolume(volume);
    }
    if (Overlays.getOverlayAtPoint({x: event.x, y: event.y}) == toggleStreamStopButton) {
        changeStream("");
    }
    if (Overlays.getOverlayAtPoint({x: event.x, y: event.y}) == toggleIncreaseVolumeButton) {
        volume += 0.25;
        changeVolume(volume);
    }
    if (Overlays.getOverlayAtPoint({x: event.x, y: event.y}) == toggleDecreaseVolumeButton) {
        volume -= 0.25;
        changeVolume(volume);
    }
}

// Function checking bool if in proper zone.
function isOurZone(properties) {
    return stream != "" && stream != undefined && properties.type == "Zone";
}

// Function to toggle visibile the overlay.
function toggleVisible(newVisibility) {
    if (newVisibility != visible) {
        visible = newVisibility;
        Overlays.editOverlay(toggleStreamPlayButton, {visible: visible});
        Overlays.editOverlay(toggleStreamStopButton, {visible: visible});
        Overlays.editOverlay(toggleIncreaseVolumeButton, {visible: visible});
        Overlays.editOverlay(toggleDecreaseVolumeButton, {visible: visible});
    }
}

// Procedure to check to see if you within a zone with a given stream.
var entities = Entities.findEntities(MyAvatar.position, 0.1);
for (var i in entities) {
    var properties = Entities.getEntityProperties(entities[i]);
    if (properties.type == "Zone") {
        print("Entered zone: " + JSON.stringify(entities[i]));
        stream = JSON.parse(properties.userData).stream;
        if (isOurZone(properties)) {
            print("Entered zone " + JSON.stringify(entities[i]) + " with stream: " + stream);
            lastZone = properties.name;
            toggleVisible(true);
        }
    }
}
// Function to check if avatar is in proper domain. 
Window.domainChanged.connect(function() {
    Script.stop();
});

// Function to check if avatar is within zone.
Entities.enterEntity.connect(function(entityID) {
    print("Entered..." + JSON.stringify(entityID));
    var properties = Entities.getEntityProperties(entityID);
    stream = JSON.parse(properties.userData).stream;
    if(isOurZone(properties))
    {
      lastZone = properties.name;
      toggleVisible(true);
    }
})

// Function to check if avatar is leaving zone.
Entities.leaveEntity.connect(function(entityID) {
    print("Left..." + JSON.stringify(entityID));
    var properties = Entities.getEntityProperties(entityID);
    if (properties.name == lastZone && properties.type == "Zone") {
        print("Leaving Zone!");
        toggleVisible(false);
        changeStream("");
    }
})

// Function to delete overlays upon exit.
function onScriptEnding() {
    Overlays.deleteOverlay(toggleStreamPlayButton);
    Overlays.deleteOverlay(toggleStreamStopButton);
    Overlays.deleteOverlay(toggleIncreaseVolumeButton);
    Overlays.deleteOverlay(toggleDecreaseVolumeButton);
    changeStream("");
    streamWindow.deleteLater();
}

// Connect mouse and hide WebWindow.
Controller.mousePressEvent.connect(mousePressEvent);
streamWindow.setVisible(false);

// Call function upon ending script.
Script.scriptEnding.connect(onScriptEnding);
