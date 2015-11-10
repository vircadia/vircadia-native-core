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

HIFI_PUBLIC_BUCKET = "http://s3.amazonaws.com/hifi-public/";

// Set the following variables to the values needed
var clip_url = "https://dl.dropboxusercontent.com/u/14127429/Clips/recording1.hfr";
var playFromCurrentLocation = true;
var useDisplayName = true;
var useAttachments = true;
var useAvatarModel = true;

// ID of the agent. Two agents can't have the same ID.
var id = 0;

// Set avatar model URL
Avatar.skeletonModelURL = "https://hifi-public.s3.amazonaws.com/marketplace/contents/d029ae8d-2905-4eb7-ba46-4bd1b8cb9d73/4618d52e711fbb34df442b414da767bb.fst?1427170144";
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

var COLORS = [];
COLORS[PLAY] = { red: PLAY, green: 0,  blue: 0 };
COLORS[PLAY_LOOP] = { red: PLAY_LOOP, green: 0,  blue: 0 };
COLORS[STOP] = { red: STOP, green: 0,  blue: 0 };
COLORS[SHOW] = { red: SHOW, green: 0,  blue: 0 };
COLORS[HIDE] = { red: HIDE, green: 0,  blue: 0 };
COLORS[LOAD] = { red: LOAD, green: 0,  blue: 0 };

controlEntityPosition.x += id * controlEntitySize;

Avatar.loadRecording(clip_url);

Avatar.setPlayFromCurrentLocation(playFromCurrentLocation);
Avatar.setPlayerUseDisplayName(useDisplayName);
Avatar.setPlayerUseAttachments(useAttachments);
Avatar.setPlayerUseHeadModel(false);
Avatar.setPlayerUseSkeletonModel(useAvatarModel);

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
    clip_url = controlEntity.userData;
    
    if (controlEntity === null ||
        controlEntity.position.x !== controlEntityPosition.x ||
        controlEntity.position.y !== controlEntityPosition.y ||
        controlEntity.position.z !== controlEntityPosition.z ||
        controlEntity.dimensions.x !== controlEntitySize) {
        return DO_NOTHING;
    }
    
    for (i in COLORS) {
        if (controlEntity.color.red === COLORS[i].red &&
            controlEntity.color.green === COLORS[i].green &&
            controlEntity.color.blue === COLORS[i].blue) {
            Entities.deleteEntity(controlEntity.id);
            return parseInt(i);
        }
    }
    
    return DO_NOTHING;
}

count = 100; // This is necessary to wait for the audio mixer to connect
function update(event) {
    EntityViewer.queryOctree();
    if (count > 0) {
        count--;
        return;
    }
    
    
    var controlEntity = Entities.findClosestEntity(controlEntityPosition, controlEntitySize);
    var action = getAction(Entities.getEntityProperties(controlEntity));
    
    switch(action) {
        case PLAY:
            print("Play");
            if (!Agent.isAvatar) {
                Agent.isAvatar = true;
            }
            if (!Avatar.isPlaying()) {
                Avatar.startPlaying();
            }
            Avatar.setPlayerLoop(false);
            break;
        case PLAY_LOOP:
            print("Play loop");
            if (!Agent.isAvatar) {
                Agent.isAvatar = true;
            }
            if (!Avatar.isPlaying()) {
                Avatar.startPlaying();
            }
            Avatar.setPlayerLoop(true);
            break;
        case STOP:
            print("Stop");
            if (Avatar.isPlaying()) {
                Avatar.stopPlaying();
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
            if (Avatar.isPlaying()) {
                Avatar.stopPlaying();
            }
            Agent.isAvatar = false;
            break;
        case LOAD:
            print("Load");            
            if(clip_url !== null) {
                Avatar.loadRecording(clip_url);
            }
            break;
        case DO_NOTHING:
            break;
        default:
            print("Unknown action: " + action);
            break;
    }
    
    if (Avatar.isPlaying()) {
        Avatar.play();
    }
}

Script.update.connect(update);
setupEntityViewer();
