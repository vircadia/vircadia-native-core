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
// See crowd-agent.js

var version = 3;
var label = "summon";
function debug() {
    print.apply(null, [].concat.apply([label, version], [].map.call(arguments, JSON.stringify)));
}

var MINIMUM_AVATARS = 25; // We will summon agents to produce this many total. (Of course, there might not be enough agents.)
var N_LISTENING = MINIMUM_AVATARS - 1;
var AVATARS_CHATTERING_AT_ONCE = 4; // How many of the agents should we request to play SOUND_DATA at once.

var initialBubble = Users.getIgnoreRadiusEnabled();
debug('startup seeking:', MINIMUM_AVATARS, 'listening:', N_LISTENING, 'chattering:', AVATARS_CHATTERING_AT_ONCE, 'had bubble:', initialBubble);

// If we add or remove things too quickly, we get problems (e.g., audio, fogbugz 2095).
// For now, spread them out this timing apart.
var SPREAD_TIME_MS = 500;

var DENSITY = 0.3; // square meters per person. Some say 10 sq ft is arm's length (0.9m^2), 4.5 is crowd (0.4m^2), 2.5 is mosh pit (0.2m^2).
var SOUND_DATA = {url: Script.getExternalPath(Script.ExternalPaths.HF_Content, "/howard/sounds/piano1.wav")};
var NEXT_SOUND_SPREAD = 500; // millisecond range of how long to wait after one sound finishes, before playing the next
var ANIMATION_DATA = {
    "url": Script.getExternalPath(Script.ExternalPaths.HF_Content, "/howard/resources/avatar/animations/idle.fbx"),
    // "url": Script.getExternalPath(Script.ExternalPaths.HF_Content, "/howard/resources/avatar/animations/walk_fwd.fbx"), // alternative example
    "startFrame": 0.0,
    "endFrame": 300.0,
    "timeScale": 1.0,
    "loopFlag": true
};

var spread = Math.sqrt(MINIMUM_AVATARS * DENSITY); // meters
var turnSpread = 90; // How many degrees should turn from front range over.

function coord() { return (Math.random() * spread) - (spread / 2); }  // randomly distribute a coordinate zero += spread/2.
function contains(array, item) { return array.indexOf(item) >= 0; }
function without(array, itemsToRemove) { return array.filter(function (item) { return !contains(itemsToRemove, item); }); }
function nextAfter(array, id) { // Wrapping next element in array after id.
    var index = array.indexOf(id) + 1;
    return array[(index >= array.length) ? 0 : index];
}

var summonedAgents = [];
var chattering = [];
var nListening = 0;
var accumulatedDelay = 0;
var MESSAGE_CHANNEL = "io.highfidelity.summon-crowd";
function messageSend(message) {
    Messages.sendMessage(MESSAGE_CHANNEL, JSON.stringify(message));
}
function messageHandler(channel, messageString, senderID) {
    if (channel !== MESSAGE_CHANNEL) {
        return;
    }
    debug('message', channel, messageString, senderID);
    if (MyAvatar.sessionUUID === senderID) { // ignore my own
        return;
    }
    var message = {};
    try {
        message = JSON.parse(messageString);
    } catch (e) {
		
    }
    switch (message.key) {
    case "hello":
        Script.setTimeout(function () {
            // There can be avatars we've summoned that do not yet appear in the AvatarList.
            var avatarIdentifiers = without(AvatarList.getAvatarIdentifiers(), summonedAgents);
            var nSummoned = summonedAgents.length;
            debug('present', avatarIdentifiers, summonedAgents);
            if ((nSummoned + avatarIdentifiers.length) < MINIMUM_AVATARS ) {
                var chatter = chattering.length < AVATARS_CHATTERING_AT_ONCE;
                var listen = nListening < N_LISTENING;
                if (chatter) {
                    chattering.push(senderID);
                }
                if (listen) {
                    nListening++;
                }
                summonedAgents.push(senderID);
                messageSend({
                    key: 'SUMMON',
                    rcpt: senderID,
                    displayName: "crowd " + nSummoned + " " + senderID,
                    position: Vec3.sum(MyAvatar.position, {x: coord(), y: 0, z: coord()}),
                    orientation: Quat.fromPitchYawRollDegrees(0, Quat.safeEulerAngles(MyAvatar.orientation).y + (turnSpread * (Math.random() - 0.5)), 0),
                    soundData: chatter && SOUND_DATA,
                    listen: listen,
                    skeletonModelURL: Script.getExternalPath(Script.ExternalPaths.HF_Content, "/howard/resources/meshes/defaultAvatar_full.fst"),
                    animationData: ANIMATION_DATA
                });
            }
        }, accumulatedDelay);
        accumulatedDelay += SPREAD_TIME_MS; // assume we'll get all the hello responses more or less together.
        break;
    case "finishedSound": // Give someone else a chance.
        chattering = without(chattering, [senderID]);
        Script.setTimeout(function () {
            messageSend({
                key: 'SUMMON',
                rcpt: nextAfter(without(summonedAgents, chattering), senderID),
                soundData: SOUND_DATA
            });
        }, Math.random() * NEXT_SOUND_SPREAD);
        break;
    case "HELO":
        Window.alert("Someone else is summoning avatars.");
        break;
    default:
        print("crowd summon.js received unrecognized message");
    }
}
Messages.subscribe(MESSAGE_CHANNEL);
Messages.messageReceived.connect(messageHandler);
Script.scriptEnding.connect(function () {
    debug('stopping agents', summonedAgents);
    Users.requestsDomainListData = false;
    if (initialBubble && !Users.getIgnoreRadiusEnabled()) { Users.toggleIgnoreRadius(); }
    Messages.messageReceived.disconnect(messageHandler); // don't respond to any messages during shutdown
    accumulatedDelay = 0;
    summonedAgents.forEach(function (id) {
        messageSend({key: 'STOP', rcpt: id, delay: accumulatedDelay});
        accumulatedDelay += SPREAD_TIME_MS;
    }); 
    debug('agents stopped');
    Messages.unsubscribe(MESSAGE_CHANNEL);
    debug('unsubscribed');
});

Users.requestsDomainListData = true; // Get avatar data for the whole domain, even if not in our view.
if (initialBubble) { Users.toggleIgnoreRadius(); }
messageSend({key: 'HELO'}); // Ask agents to report in now.
Script.setTimeout(function () {
    var total = AvatarList.getAvatarIdentifiers().length;
    if (0 === summonedAgents.length) {
        Window.alert("No agents reported.\n\Please run " + MINIMUM_AVATARS + " instances of\n\
https://cdn-1.vircadia.com/eu-c-1/howard/scripts/tests/performance/crowd-agent.js?v=someDate\n\
on your domain server.");
    } else if (total < MINIMUM_AVATARS) {
        Window.alert("Only " + summonedAgents.length + " agents reported. Now missing " + (MINIMUM_AVATARS - total) + " avatars, total.");
    }
    Users.requestsDomainListData = false;
}, MINIMUM_AVATARS * SPREAD_TIME_MS )
