//
// #20628:  JS Stream Player Domain-Zone
// *************************************
//
// Created by Kevin M. Thomas and Thoys 07/20/15.
// Copyright 2015 High Fidelity, Inc.
// kevintown.net
//
// JavaScript for the High Fidelity interface that creates a stream player with a UI for playing a domain-zone specificed stream URL in addition to play, stop and volume functionality which is resident only in the domain-zone.
//
// Distributed under the Apache License, Version 2.0.
// See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
 
 
// Declare variables and set up new WebWindow.
var stream = "http://listen.radionomy.com/80sMixTape";
var volume;
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
    return properties.type == "Zone" && properties.name == "Zone2";
}

// Function checking if avatar is in zone.
function isInZone() {
    var entities = Entities.findEntities(MyAvatar.position, 10000);

    for (var i in entities) {
        var properties = Entities.getEntityProperties(entities[i]);

        if (isOurZone(properties)) {
            var minX = properties.position.x - (properties.dimensions.x / 2);
            var maxX = properties.position.x + (properties.dimensions.x / 2);
            var minY = properties.position.y - (properties.dimensions.y / 2);
            var maxY = properties.position.y + (properties.dimensions.y / 2);
            var minZ = properties.position.z - (properties.dimensions.z / 2);
            var maxZ = properties.position.z + (properties.dimensions.z / 2);

            if (MyAvatar.position.x >= minX && MyAvatar.position.x <= maxX && MyAvatar.position.y >= minY && MyAvatar.position.y <= maxY && MyAvatar.position.z >= minZ && MyAvatar.position.z <= maxZ) {
                return true;
            }
        }
    }
    return false;
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
 
// Function to check if avatar is in proper domain. 
Window.domainChanged.connect(function() {
    if (Window.location.hostname != "Music" && Window.location.hostname != "LiveMusic") {
        Script.stop();
    } 
});

// Function to check if avatar is within zone.
Entities.enterEntity.connect(function(entityID) { 
    print("Entered..." + JSON.stringify(entityID)); 
    var properties = Entities.getEntityProperties(entityID);
    if (isOurZone(properties)) {
        print("Entering Zone!");
        toggleVisible(true);
    }
})
 
// Function to check if avatar is leaving zone.
Entities.leaveEntity.connect(function(entityID) { 
    print("Left..." + JSON.stringify(entityID)); 
    var properties = Entities.getEntityProperties(entityID);
    if (isOurZone(properties)) {
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

// Function call to ensure that if you log in to the zone visibility is true.
if (isInZone()) {
    toggleVisible(true);
}

// Connect mouse and hide WebWindow.
Controller.mousePressEvent.connect(mousePressEvent);
streamWindow.setVisible(false);
 
// Call function upon ending script.
Script.scriptEnding.connect(onScriptEnding);