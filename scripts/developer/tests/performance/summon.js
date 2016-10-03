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

var version = 1;
var label = "summon";
function debug() {
    print.apply(null, [].concat.apply([label, version], [].map.call(arguments, JSON.stringify)));
}
var MINIMUM_AVATARS = 25;
var DENSITY = 0.3; // square meters per person. Some say 10 sq ft is arm's length (0.9m^2), 4.5 is crowd (0.4m^2), 2.5 is mosh pit (0.2m^2).
var spread = Math.sqrt(MINIMUM_AVATARS * DENSITY); // meters
var turnSpread = 90; // How many degrees should turn from front range over.

function coord() { return (Math.random() * spread) - (spread / 2); }  // randomly distribute a coordinate zero += spread/2.


var summonedAgents = [];
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
    var message = {}, avatarIdentifiers;
    try {
        message = JSON.parse(messageString);
    } catch (e) {
        print(e);
    }
    switch (message.key) {
    case "hello":
        // There can be avatars we've summoned that do not yet appear in the AvatarList.
        avatarIdentifiers = AvatarList.getAvatarIdentifiers().filter(function (id) { return summonedAgents.indexOf(id) === -1; });
        debug('present', avatarIdentifiers, summonedAgents);
        if ((summonedAgents.length + avatarIdentifiers.length) < MINIMUM_AVATARS ) {
            summonedAgents.push(senderID);
            messageSend({
                key: 'SUMMON',
                rcpt: senderID,
                position: Vec3.sum(MyAvatar.position, {x: coord(), y: 0, z: coord()}),
                orientation: Quat.fromPitchYawRollDegrees(0, Quat.safeEulerAngles(MyAvatar.orientation).y + (turnSpread * (Math.random() - 0.5)), 0)/*,
                // No need to specify skeletonModelURL
                //skeletonModelURL: "file:///c:/Program Files/High Fidelity Release/resources/meshes/being_of_light/being_of_light.fbx",
                //skeletonModelURL: "file:///c:/Program Files/High Fidelity Release/resources/meshes/defaultAvatar_full.fst"/,
                animationData: { // T-pose until we get animations working again.
                    "url": "file:///C:/Program Files/High Fidelity Release/resources/avatar/animations/idle.fbx",
                    //"url": "file:///c:/Program Files/High Fidelity Release/resources/avatar/animations/walk_fwd.fbx",
                    "startFrame": 0.0,
                    "endFrame": 300.0,
                    "timeScale": 1.0,
                    "loopFlag": true
                }*/
            });
        }
        break;
    case "HELO":
        Window.alert("Someone else is summoning avatars.");
        break;
    default:
        print("crowd-agent received unrecognized message:", messageString);
    }
}
Messages.subscribe(MESSAGE_CHANNEL);
Messages.messageReceived.connect(messageHandler);
Script.scriptEnding.connect(function () {
    debug('stopping agents', summonedAgents);
    summonedAgents.forEach(function (id) { messageSend({key: 'STOP', rcpt: id}); });
    debug('agents stopped');
    Script.setTimeout(function () {
        Messages.messageReceived.disconnect(messageHandler);
        Messages.unsubscribe(MESSAGE_CHANNEL);
        debug('unsubscribed');
    }, 500);
});

messageSend({key: 'HELO'}); // Ask agents to report in now, before we start the tribbles.
