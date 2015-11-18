//
//  playbackAgents.js
//  acScripts
//
//  Created by Edgar Pironti on 11/17/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

// Set the following variables to the values needed
var channel = "PlaybackChannel1";
var clip_url = null;
var playFromCurrentLocation = true;
var useDisplayName = true;
var useAttachments = true;
var useAvatarModel = true;

// ID of the agent. Two agents can't have the same ID.
var id = 0;

// Set position/orientation/scale here if playFromCurrentLocation is true
Avatar.position = { x:0, y: 0, z: 0 };
Avatar.orientation = Quat.fromPitchYawRollDegrees(0, 0, 0);
Avatar.scale = 1.0;

var totalTime = 0;
var subscribed = false;
var WAIT_FOR_AUDIO_MIXER = 1;

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

function getAction(channel, message, senderID) {    

    if(subscribed) {
        var command = JSON.parse(message);
        print("I'm the agent " + id + " and I received this: ID: " + command.id_key + " Action: " + command.action_key + " URL: " + command.clip_url_key);
        
        if (command.id_key == id || command.id_key == -1) {
            if (command.action_key === 6)
                clip_url = command.clip_url_key;
            
            // If the id is -1 (broadcast) and the action is 6, in the url should be the performance file
            // with all the clips recorded in a session (not just the single clip url).
            // It has to be computed here in order to retrieve the url for the single agent. 
            // Checking the id we can assign the correct url to the correct agent.
            
            action = command.action_key;
        } else {
            action = DO_NOTHING;
        } 
        
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
}


function update(deltaTime) {   

    totalTime += deltaTime;

    if (totalTime > WAIT_FOR_AUDIO_MIXER && !subscribed) {
        Messages.subscribe(channel);
        subscribed = true;
        print("I'm the agent and I am ready to receive!")
    }
}

Script.update.connect(update);
Messages.messageReceived.connect(getAction);

