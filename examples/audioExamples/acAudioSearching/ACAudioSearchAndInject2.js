//
//  ACAudioSearchAndInject2.js
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

var SOUND_DATA_KEY = "soundKey";

var QUERY_RADIUS = 5;

EntityViewer.setKeyholeRadius(QUERY_RADIUS);
Entities.setPacketsPerSecond(6000);

Agent.isAvatar = true;

var DEFAULT_SOUND_DATA = {
    volume: 0.5,
    loop: false,
    interval: -1, // An interval of -1 means this sound only plays once (if it's non-looping) (In seconds)
    intervalSpread: 0 // amount of randomness to add to the interval
};
var MIN_INTERVAL = 0.2;

print("EBL STARTING SCRIPT");

function slowUpdate() {
    var avatars = AvatarList.getAvatarIdentifiers();
    print("AVATARS LENGTH " + avatars.length);
    for (var i = 0; i < avatars.length; i++) {
        print("In loop");
        var avatar = AvatarList.getAvatar(avatars[i]);
        EntityViewer.setPosition(avatar.position);
        EntityViewer.queryOctree();
        Script.setTimeout(function() {
            var entities = Entities.findEntities(avatar.position, QUERY_RADIUS);
            print("EBL ENTITIES LENGTH " + entities.length);
        }, 1000);
    }
}
Script.setInterval(slowUpdate, 1000);

// Script.setInterval(function() {
//     var avatars = AvatarList.getAvatarIdentifiers();
//     print("Checking avatar identifiers" + JSON.stringify(avatars));
// }, 2000)//
//  ACAudioSearchAndInject2.js
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

var SOUND_DATA_KEY = "soundKey";

var QUERY_RADIUS = 5;

EntityViewer.setKeyholeRadius(QUERY_RADIUS);
Entities.setPacketsPerSecond(6000);

Agent.isAvatar = true;

var DEFAULT_SOUND_DATA = {
    volume: 0.5,
    loop: false,
    interval: -1, // An interval of -1 means this sound only plays once (if it's non-looping) (In seconds)
    intervalSpread: 0 // amount of randomness to add to the interval
};
var MIN_INTERVAL = 0.2;

print("EBL STARTING SCRIPT");

function slowUpdate() {
    var avatars = AvatarList.getAvatarIdentifiers();
    print("AVATARS LENGTH " + avatars.length);
    for (var i = 0; i < avatars.length; i++) {
        print("In loop");
        var avatar = AvatarList.getAvatar(avatars[i]);
        EntityViewer.setPosition(avatar.position);
        EntityViewer.queryOctree();
        Script.setTimeout(function() {
            var entities = Entities.findEntities(avatar.position, QUERY_RADIUS);
            print("EBL ENTITIES LENGTH " + entities.length);
        }, 1000);
    }
}
Script.setInterval(slowUpdate, 1000);

// Script.setInterval(function() {
//     var avatars = AvatarList.getAvatarIdentifiers();
//     print("Checking avatar identifiers" + JSON.stringify(avatars));
// }, 2000)