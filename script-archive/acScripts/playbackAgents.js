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

Script.include("./AgentPoolController.js");
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
var PLAY = 1;
var PLAY_LOOP = 2;
var STOP = 3;
var LOAD = 6;

Recording.setPlayFromCurrentLocation(playFromCurrentLocation);
Recording.setPlayerUseDisplayName(useDisplayName);
Recording.setPlayerUseAttachments(useAttachments);
Recording.setPlayerUseHeadModel(false);
Recording.setPlayerUseSkeletonModel(useAvatarModel);

function agentCommand(command) {    
    if(true) {

       // var command = JSON.parse(message);
        print("I'm the agent " + this.agentUUID + " and I received this: Dest: " + command.dest_key + " Action: " + command.action_key + " URL: " + command.argument_key);
     
        switch(command.action_key) {
        case PLAY:
            print("Play");
            if (!Recording.isPlaying()) {
                Recording.startPlaying();
            }
            Recording.setPlayerLoop(false);
            break;
        case PLAY_LOOP:
            print("Play loop");
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
        case LOAD:
        {
            print("Load" + command.argument_key);  
            print("Agent #" + command.dest_key + " loading clip URL: " + command.argument_key);
                Recording.loadRecording(command.argument_key);
            print("After Load" + command.argument_key);  
            Recording.setPlayerTime(0);
         }
            break;
        default:
            print("Unknown action: " + command.action_key);
            break;

        }
    }
}

function agentHired() {
    print("Agent Hired from playbackAgents.js");
    Agent.isAvatar = true;
    Recording.stopPlaying();
    Recording.setPlayerLoop(false);
    Recording.setPlayerTime(0);
}

function agentFired() {
    print("Agent Fired from playbackAgents.js");
    Recording.stopPlaying();
    Recording.setPlayerTime(0);
    Recording.setPlayerLoop(false);
    Agent.isAvatar = false;
}


function update(deltaTime) {
    totalTime += deltaTime;
    if (totalTime > WAIT_FOR_AUDIO_MIXER) {
        if (!agentController.subscribed) {
            agentController.reset();
            agentController.onCommand = agentCommand;
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
