"use strict";
/*jslint vars: true, plusplus: true*/
/*global Agent, Avatar, Script, Entities, Vec3, Quat, print*/
//
//  crowd-agent.js
//  scripts/developer/tests/performance/
//
//  Created by Howard Stearns on 9/29/16.
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
//  Add this to domain-settings scripts url with n instances. It will lie dormant until
//  a script like summon.js calls up to n avatars to be around you.

var MESSAGE_CHANNEL = "io.highfidelity.summon-crowd";
var soundIntervalId;
var SOUND_POLL_INTERVAL = 500; // ms
var SOUND_URL = "http://howard-stearns.github.io/models/sounds/piano1.wav";

print('crowd-agent version 1');

/* Observations:
- File urls for AC scripts silently fail. Use a local server (e.g., python SimpleHTTPServer) for development.
- URLs are cached regardless of server headers. Must use cache-defeating query parameters.
- JSON.stringify(Avatar) silently fails (even when Agent.isAvatar)
*/

function startAgent(parameters) { // Can also be used to update.
    print('crowd-agent starting params', JSON.stringify(parameters), JSON.stringify(Agent));
    Agent.isAvatar = true;
    if (parameters.position) {
        Avatar.position = parameters.position;
    }
    if (parameters.orientation) {
        Avatar.orientation = parameters.orientation;
    }
    if (parameters.skeletonModelURL) {
        Avatar.skeletonModelURL = parameters.skeletonModelURL;
    }
    if (parameters.animationData) {
        data = parameters.animationData;
        Avatar.startAnimation(data.url, data.fps || 30, 1.0, (data.loopFlag === undefined) ? true : data.loopFlag, false, data.startFrame || 0, data.endFrame);
    }
    print('crowd-agent avatars started');
    Agent.isListeningToAudioStream = true;
    soundIntervalId = playSound();   
    print('crowd-agent sound loop started');
}
function playSound() {
    // Load a sound
    var sound = SoundCache.getSound(SOUND_URL);
    function loopSound(sound) {
        // since we cannot loop, for now lets just see if we are making sounds.  If
        // not, then play a sound.
        if (!Agent.isPlayingAvatarSound) {
            Agent.playAvatarSound(sound);
        }
    };
    return Script.setInterval(function() {loopSound(sound);}, SOUND_POLL_INTERVAL);
}
function stopAgent(parameters) {
    Agent.isAvatar = false;
    Agent.isListeningToAudioStream = false;
    Script.clearInterval(soundIntervalId);
    print('crowd-agent stopped', JSON.stringify(parameters), JSON.stringify(Agent));
}

function messageSend(message) {
    Messages.sendMessage(MESSAGE_CHANNEL, JSON.stringify(message));
}
function messageHandler(channel, messageString, senderID) {
    if (channel !== MESSAGE_CHANNEL) {
        return;
    }
    print('crowd-agent message', channel, messageString, senderID);
    if (Agent.sessionUUID === senderID) { // ignore my own
        return;
    }
    var message = {};
    try {
        message = JSON.parse(messageString);
    } catch (e) {
        print(e);
    }
    switch (message.key) {
    case "HELO":
        messageSend({key: 'hello'}); // Allow the coordinator to count responses and make assignments.
        break;
    case 'hello': // ignore responses (e.g., from other agents)
        break;
    case "SUMMON":
        if (message.rcpt === Agent.sessionUUID) {
            startAgent(message);
        }
        break;
    case "STOP":
        if (message.rcpt === Agent.sessionUUID) {
            stopAgent(message);
        }
        break;
    default:
        print("crowd-agent received unrecognized message:", channel, messageString, senderID);
    }
}
Messages.subscribe(MESSAGE_CHANNEL);
Messages.messageReceived.connect(messageHandler);

Script.scriptEnding.connect(function () {
    print('crowd-agent shutting down');
    Messages.messageReceived.disconnect(messageHandler);
    Messages.unsubscribe(MESSAGE_CHANNEL);
    print('crowd-agent unsubscribed');
});
