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
var soundEntityMap = {};
var soundUrls = {};

EntityViewer.setPosition({
    x: 0,
    y: 0,
    z: 0
});

EntityViewer.setKeyholeRadius(60000);
Entities.setPacketsPerSecond(6000);

var DEFAULT_SOUND_DATA = {
    volume: 0.5,
    loop: false,
    interval: -1 // An interval of -1 means this sound only plays once (if it's non-looping)
};

print("EBL STARTING AC SCRIPT");

function messageReceived(channel, message, sender) {
    
    print("EBL RECEIVED A MESSAGE FROM ENTITY: " + message);
    var entityID = JSON.parse(message).id;
    if (soundEntityMap[entityID]) {
        // We already have this entity in our sound map, so don't re-add
        return;
    }

    EntityViewer.queryOctree();
    var soundData = getEntityCustomData(SOUND_DATA_KEY, entityID);
    print("SOUND DATA " + JSON.stringify(soundData))
    if (soundData && soundData.url) {
        var soundProperties = {
            url: soundData.url,
            volume: soundData.volume || DEFAULT_SOUND_DATA.volume,
            loop: soundData.loop || DEFAULT_SOUND_DATA.loop,
            interval: soundData.interval || DEFAULT_SOUND_DATA.interval,
            readyToPlay: true,
            position: Entities.getEntityProperties(entityID, "position").position,
            timeSinceLastPlay: 0
        };
        if (!soundUrls[soundData.url]) {
            // We need to download sound before we add it to our map
            var sound = SoundCache.getSound(soundData.url);
            soundUrls[soundData.url] = sound;
            // Only add it to map once it's downloaded
            sound.ready.connect(function() {
                soundProperties.sound = sound;
                soundEntityMap[entityID] = soundProperties;
            });
        } else {
            // We already have sound downloaded, so just add it to map right away
            soundProperties.sound = soundUrls[soundData.url];
            soundEntityMap[entityID] = soundProperties;
        }
    }
}

function update(deltaTime) {
    // Go through each sound and play it if it needs to be played
    for (var potentialEntity in soundEntityMap) {
        if(!soundEntityMap.hasOwnProperty(potentialEntity)) {
            // The current property is not a direct propert of soundEntityMap
            continue;
        }
        var entity = potentialEntity;
        var soundProperties = soundEntityMap[entity];
        if (soundProperties.readyToPlay) {
            print("EBL NEEDS TO PLAY!")
            Audio.playSound(soundProperties.sound, {
                volume: soundProperties.volume,
                position: soundProperties.position,
                loop: soundProperties.loop
            });       
            soundProperties.readyToPlay = false;         
        } else if(soundProperties.loop === false && soundProperties.interval !==-1) {
            // We need to check all of our entities that are not looping but have an interval associated with them
            // to see if it's time for them to play again
            soundProperties.timeSinceLastPlay += deltaTime;
            if (soundProperties.timeSinceLastPlay > soundProperties.interval) {
                print ("EBL TIME TO PLAY AGAIN!");
                soundProperties.readyToPlay = true;
                soundProperties.timeSinceLastPlay = 0;
            }

        }
    }
}

Script.update.connect(update);
Messages.subscribe(MESSAGE_CHANNEL);
Messages.messageReceived.connect(messageReceived);