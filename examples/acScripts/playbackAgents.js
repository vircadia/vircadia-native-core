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
Agent.isAvatar = true;
Script.include("./AgentPoolControler.js");

// Set the following variables to the values needed
var playFromCurrentLocation = true;
var useDisplayName = true;
var useAttachments = true;
var useAvatarModel = true;

var agentController = new AgentController();

// ID of the agent. Two agents can't have the same ID.
//var UNKNOWN_AGENT_ID = -2;
//var id = UNKNOWN_AGENT_ID;  // unknown until aknowledged

// The time between alive messages on the command channel
/*var timeSinceLastAlive = 0;
var ALIVE_PERIOD = 5;
var NUM_CYCLES_BEFORE_RESET = 5;
var notifyAlive = false;
*/

// Set position/orientation/scale here if playFromCurrentLocation is true
Avatar.position = { x:0, y: 0, z: 0 };
Avatar.orientation = Quat.fromPitchYawRollDegrees(0, 0, 0);
Avatar.scale = 1.0;

var totalTime = 0;
var subscribed = false;
var WAIT_FOR_AUDIO_MIXER = 1;

// Script. DO NOT MODIFY BEYOND THIS LINE.
var ALIVE = -1;
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
                           
            action = command.action_key;
            print("That command was for me! Agent with id: " + id);
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
            if (!Agent.isAvatar) {
                Agent.isAvatar = true;
            }         
            if(command.clip_url_key !== null) {
                print("Agent #" + id + " loading clip URL: " + command.clip_url_key);
                Recording.loadRecording(command.clip_url_key);
            } else {
                 print("Agent #" + id + " loading clip URL is NULL, nothing happened"); 
            }
            break;
        case MASTER_ALIVE:
            print("Alive");
            notifyAlive = true;
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
        if (!agentController.subscribed) {
            agentController.reset();
        }
    }
    /*
        
    totalTime += deltaTime;
  if (totalTime > WAIT_FOR_AUDIO_MIXER) {
        if (!subscribed) {
            Messages.subscribe(commandChannel); // command channel
            Messages.subscribe(announceIDChannel); // id announce channel
            subscribed = true;
            print("I'm the agent and I am ready to receive!");
        }
        if (subscribed && id == UNKNOWN_AGENT_ID) {

            Messages.sendMessage(announceIDChannel, "ready");
        }
    }

    if (subscribed && id != UNKNOWN_AGENT_ID) {
        timeSinceLastAlive += deltaTime;
        if (notifyAlive) {
            timeSinceLastAlive = 0;
            notifyAlive = false;
            print("Master Alive");            
        } else if (timeSinceLastAlive > NUM_CYCLES_BEFORE_RESET * ALIVE_PERIOD) {
            print("Master Lost, reseting Agent");
            if (Recording.isPlaying()) {
                Recording.stopPlaying();
            }
            Agent.isAvatar = false;
            id = UNKNOWN_AGENT_ID;           
        }
    }*/

    agentController.update(deltaTime);
}
/*
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
});*/

function scriptEnding() {
   
    agentController.destroy();
}


Script.update.connect(update);
Script.scriptEnding.connect(scriptEnding);
