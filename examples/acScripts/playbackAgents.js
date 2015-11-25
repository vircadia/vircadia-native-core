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
var agentController = new AgentController();

// Set the following variables to the values needed
var playFromCurrentLocation = true;
var useDisplayName = true;
var useAttachments = true;
var useAvatarModel = true;


// Set position/orientation/scale here if playFromCurrentLocation is true
Avatar.position = { x:0, y: 0, z: 0 };
Avatar.orientation = Quat.fromPitchYawRollDegrees(0, 0, 0);
Avatar.scale = 1.0;

var totalTime = 0;
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

function getAction(command) {    
    if(true) {

       // var command = JSON.parse(message);
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
            if(command.argument_key !== null) {
                print("Agent #" + id + " loading clip URL: " + command.argument_key);
                Recording.loadRecording(command.argument_key);
            } else {
                 print("Agent #" + id + " loading clip URL is NULL, nothing happened"); 
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

function agentHired() {
    print("Agent Hired from playbackAgents.js");
}

function agentFired() {
    print("Agent Fired from playbackAgents.js");
}


function update(deltaTime) {
    totalTime += deltaTime;
    if (totalTime > WAIT_FOR_AUDIO_MIXER) {
        if (!agentController.subscribed) {
            agentController.reset();
            agentController.onCommand = getAction;
            agentController.onHired = agentHired;
            agentController.onFired = agentFired;
        }
    }

    agentController.update(deltaTime);
}


function scriptEnding() {
   
    agentController.destroy();
}


Script.update.connect(update);
Script.scriptEnding.connect(scriptEnding);
