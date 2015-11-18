//
//  ControlledAC.js
//  examples
//
//  Created by Clément Brisset on 8/28/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//


// Set the following variables to the values needed
var clip_url = null;
var playFromCurrentLocation = true;
var useDisplayName = true;
var useAttachments = true;
var useAvatarModel = true;

// ID of the agent. Two agents can't have the same ID.
var id = 0;

// Set position/orientation/scale here if playFromCurrentLocation is true
Avatar.position = { x:1, y: 1, z: 1 };
Avatar.orientation = Quat.fromPitchYawRollDegrees(0, 0, 0);
Avatar.scale = 1.0;

// Those variables MUST be common to every scripts
var controlEntitySize = 0.25;
var controlEntityPosition = { x: 0, y: 0, z: 0 };

// Script. DO NOT MODIFY BEYOND THIS LINE.
var DO_NOTHING = 0;
var PLAY = 1;
var PLAY_LOOP = 2;
var STOP = 3;
var SHOW = 4;
var HIDE = 5;
var LOAD = 6;

Recording.setPlayFromCurrentLocation(playFromCurrentLocation);
Recording.setPlayerUseDisplayName(useDisplayName);
Recording.setPlayerUseAttachments(useAttachments);
Recording.setPlayerUseHeadModel(false);
Recording.setPlayerUseSkeletonModel(useAvatarModel);

function setupEntityViewer() {
    var entityViewerOffset = 10;
    var entityViewerPosition = { x: controlEntityPosition.x - entityViewerOffset,
        y: controlEntityPosition.y, z: controlEntityPosition.z };
    var entityViewerOrientation = Quat.fromPitchYawRollDegrees(0, -90, 0);
    
    EntityViewer.setPosition(entityViewerPosition);
    EntityViewer.setOrientation(entityViewerOrientation);
    EntityViewer.queryOctree();
}

function getAction(controlEntity) {
     if (controlEntity === null) {
         return DO_NOTHING;
     }
        
    var userData = JSON.parse(Entities.getEntityProperties(controlEntity, ["userData"]).userData);
    
    var uD_id = userData.idKey.uD_id;
    var uD_action = userData.actionKey.uD_action;
    var uD_url = userData.clipKey.uD_url;
    
    Entities.deleteEntity((Entities.getEntityProperties(controlEntity)).id);

    if (uD_id === id || uD_id === -1) {
        if (uD_action === 6)
            clip_url = uD_url;
        
        return uD_action;
    } else {
        return DO_NOTHING;
    } 
}

count = 100; // This is necessary to wait for the audio mixer to connect
function update(event) {
    EntityViewer.queryOctree();
    if (count > 0) {
        count--;
        return;
    }
    
    
    var controlEntity = Entities.findClosestEntity(controlEntityPosition, controlEntitySize);
    var action = getAction(controlEntity);
    
    switch(action) {
        case PLAY:
            print("Play");
            if (!Agent.isAvatar) {
                Agent.isAvatar = true;
            }
            if (!Recording.isPlaying()) {
                Recording.startPlaying();
            }
            Recording.setPlayerLoop(false);
            break;
        case PLAY_LOOP:
            print("Play loop");
            if (!Agent.isAvatar) {
                Agent.isAvatar = true;
            }
            if (!Recording.isPlaying()) {
                Recording.startPlaying();
            }
            Recording.setPlayerLoop(true);
            break;
        case STOP:
            print("Stop");
            if (Recording.isPlaying()) {
                Recording.stopPlaying();
            }
            break;
        case SHOW:
            print("Show");
            if (!Agent.isAvatar) {
                Agent.isAvatar = true;
            }
            break;
        case HIDE:
            print("Hide");
            if (Recording.isPlaying()) {
                Recording.stopPlaying();
            }
            Agent.isAvatar = false;
            break;
        case LOAD:
            print("Load");            
            if(clip_url !== null) {
                Recording.loadRecording(clip_url);
            }
            break;
        case DO_NOTHING:
            break;
        default:
            print("Unknown action: " + action);
            break;
    }
    
    if (Recording.isPlaying()) {
        Recording.play();
    }
}

Script.update.connect(update);
setupEntityViewer();
