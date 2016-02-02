//
//  ACAudioSearchAndInject.js
//  audio
//
//  Created by Eric Levin 2/1/2016
//  Copyright 2016 High Fidelity, Inc.

//  This AC script constantly searches for entities with a special userData field that specifies audio settings, and then
//  injects the sound with the specified URL with other specified settings (playback volume) or playback at interval, or random interval
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

Script.include("https://rawgit.com/highfidelity/hifi/master/examples/libraries/utils.js");

var ENTITY_QUERY_INTERVAL = 2000;
var SOUND_DATA_KEY = "soundKey";
var MESSAGE_CHANNEL = "Hifi-Sound-Entity";

// Map of all sound entities in domain- key is entity id, value is data
var soundEntities = {};

EntityViewer.setPosition({
    x: 0,
    y: 0,
    z: 0
});

EntityViewer.setKeyholeRadius(60000);
Entities.setPacketsPerSecond(6000);

var DEFAULT_SOUND_DATA = {
    volume: 0.5,
    loop: false
};

print("EBL STARTING AC SCRIPT");

function messageReceived(channel, message, sender) {
    

}

Messages.subscribe(MESSAGE_CHANNEL);
Messages.messageReceived.connect(messageReceived);