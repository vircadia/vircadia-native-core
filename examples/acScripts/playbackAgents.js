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
var commandChannel = "com.highfidelity.PlaybackChannel1";
var clip_url = null;
var playFromCurrentLocation = true;
var useDisplayName = true;
var useAttachments = true;
var useAvatarModel = true;

// ID of the agent. Two agents can't have the same ID.
var announceIDChannel = "com.highfidelity.playbackAgent.announceID";
var UNKNOWN_AGENT_ID = -2;
var id = UNKNOWN_AGENT_ID;  // unknown until aknowledged

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
            if (command.action_key === 6) {
                
                clip_url = command.clip_url_key;
                
                if (command.id_key == -1) {
                    Assets.downloadData(clip_url, function (data) {
                        var myJSONObject = JSON.parse(data);
                        var hash = myJSONObject.avatarClips[id];
                    });
                    
                    clip_url = hash;
                }
            }
                
            
            action = command.action_key;
            print("That command was for me!");
            print("My clip is: " + clip_url);
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

    if (totalTime > WAIT_FOR_AUDIO_MIXER) {
        if (!subscribed) {
            Messages.subscribe(commandChannel); // command channel
            Messages.subscribe(announceIDChannel); // id announce channel
            subscribed = true;
            print("I'm the agent and I am ready to receive!");
        }
        if (subscribed && id == UNKNOWN_AGENT_ID) {
            print("sending ready, id:" + id);
            Messages.sendMessage(announceIDChannel, "ready");
        }
    }

}

Messages.messageReceived.connect(function (channel, message, senderID) {
    if (channel == announceIDChannel && message != "ready") {
        // If I don't yet know if my ID has been recieved, then check to see if the master has acknowledged me
        if (id == UNKNOWN_AGENT_ID) {
            var parts = message.split(".");
            var agentID = parts[0];
            var agentIndex = parts[1];
            if (agentID == Agent.sessionUUID) {
                id = agentIndex;
                Messages.unsubscribe(announceIDChannel); // id announce channel
            }
        }
    }
    if (channel == commandChannel) {
        getAction(channel, message, senderID);
    }
});

Script.update.connect(update);

