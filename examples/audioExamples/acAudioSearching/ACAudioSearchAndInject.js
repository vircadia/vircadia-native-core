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

var SOUND_DATA_KEY = "soundKey";
var MESSAGE_CHANNEL = "Hifi-Sound-Entity";

// Map of all sound entities in domain- key is entity id, value is sound data
var soundEntityMap = {};
// Map of sound urls so a sound that's already been downloaded from one entity is not re-downloaded if 
// another entity with same sound url is discovered
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
    interval: -1, // An interval of -1 means this sound only plays once (if it's non-looping) (In seconds)
    intervalSpread: 0 // amount of randomness to add to the interval
};
var MIN_INTERVAL = 0.2;

function messageReceived(channel, message, sender) {
    print("EBL MESSAGE RECIEVED");
    var entityID = JSON.parse(message).id;
    if (soundEntityMap[entityID]) {
        // We already have this entity in our sound map, so don't re-add
        return;
    }

    EntityViewer.queryOctree();
    Script.setTimeout(function() {
        handleIncomingEntity(entityID);
    }, 2000);

}

function handleIncomingEntity(entityID) {
    var soundData = getEntityCustomData(SOUND_DATA_KEY, entityID);
    print("SOUND DATA " + JSON.stringify(soundData));
    if (soundData && soundData.url) {
        var soundProperties = {
            url: soundData.url,
            volume: soundData.volume || DEFAULT_SOUND_DATA.volume,
            loop: soundData.loop || DEFAULT_SOUND_DATA.loop,
            interval: soundData.interval || DEFAULT_SOUND_DATA.interval,
            intervalSpread: soundData.intervalSpread || DEFAULT_SOUND_DATA.intervalSpread,
            readyToPlay: true,
            position: Entities.getEntityProperties(entityID, "position").position,
            timeSinceLastPlay: 0
        };
        if (soundProperties.interval !== -1) {
            soundProperties.currentInterval = soundProperties.interval + randFloat(-soundProperties.intervalSpread, soundProperties.intervalSpread);
            soundProperties.currentInterval = Math.max(MIN_INTERVAL, soundProperties.currentInterval);
        }
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
        if (!soundEntityMap.hasOwnProperty(potentialEntity)) {
            // The current property is not a direct propert of soundEntityMap so ignore it
            continue;
        }
        var entity = potentialEntity;
        var soundProperties = soundEntityMap[entity];
        if (soundProperties.readyToPlay) {
            var newPosition = Entities.getEntityProperties(entity, "position").position
            print("EBL SHOULD BE PLAYING SOUND!")
            Audio.playSound(soundProperties.sound, {
                volume: soundProperties.volume,
                position: newPosition,
                loop: soundProperties.loop
            });
            soundProperties.readyToPlay = false;
        } else if (soundProperties.loop === false && soundProperties.interval !== -1) {
            // We need to check all of our entities that are not looping but have an interval associated with them
            // to see if it's time for them to play again
            soundProperties.timeSinceLastPlay += deltaTime;
            if (soundProperties.timeSinceLastPlay > soundProperties.currentInterval) {
                soundProperties.readyToPlay = true;
                soundProperties.timeSinceLastPlay = 0;
                // Now lets get our next current interval
                soundProperties.currentInterval = soundProperties.interval + randFloat(-soundProperties.intervalSpread, soundProperties.intervalSpread);
                soundProperties.currentInterval = Math.max(MIN_INTERVAL, soundProperties.currentInterval);
            }
        }
    }
}

Script.update.connect(update);
Messages.subscribe(MESSAGE_CHANNEL);
Messages.messageReceived.connect(messageReceived);