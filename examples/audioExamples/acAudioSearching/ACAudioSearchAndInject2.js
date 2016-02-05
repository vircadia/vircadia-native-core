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
var MIN_PLAY_INTERVAL = 0.2;

var UPDATE_TIME = 2000;
var EXPIRATION_TIME = 7000;

var soundEntityMap = {};
var soundUrls = {};

print("EBL STARTING SCRIPT");

function slowUpdate() {
    var avatars = AvatarList.getAvatarIdentifiers();
    for (var i = 0; i < avatars.length; i++) {
        var avatar = AvatarList.getAvatar(avatars[i]);
        var avatarPosition = avatar.position;
        if (!avatarPosition) {
            print("EBL This avatars been DELETED *******! Don't add it to list");
            continue;
        }
        EntityViewer.setPosition(avatarPosition);
        EntityViewer.queryOctree();
        Script.setTimeout(function() {
            var entities = Entities.findEntities(avatar.position, QUERY_RADIUS);
            handleFoundSoundEntities(entities);
        }, 1000);
    }
    handleActiveSoundEntities();
}

function handleActiveSoundEntities() {
    // Go through all our sound entities, if they have passed expiration time, remove them from map
    for (var potentialSoundEntity in soundEntityMap) {
        if (!soundEntityMap.hasOwnProperty(potentialSoundEntity)) {
            // The current property is not a direct property of soundEntityMap so ignore it
            continue;
        }
        var soundEntity = potentialSoundEntity;
        var soundProperties = soundEntityMap[soundEntity];
        soundProperties.timeWithoutAvatarInRange += UPDATE_TIME;
        if (soundProperties.timeWithoutAvatarInRange > EXPIRATION_TIME && soundProperties.soundInjector) {
            // An avatar hasn't been within range of this sound entity recently, so remove it from map
            print("NO AVATARS HAVE BEEN AROUND FOR A WHILE SO REMOVE THIS SOUND FROM SOUNDMAP!");
            soundProperties.soundInjector.stop();
            delete soundEntityMap[soundEntity];
        } else if(soundProperties.isDownloaded) {
            print("WERE DOWNLOADED");
            // If this sound hasn't expired yet, we want to potentially play it!
            if (soundProperties.readyToPlay) {
                print("WERE READY TO PLAY")
                var newPosition = Entities.getEntityProperties(soundEntity, "position").position;
                if (!soundProperties.soundInjector) {
                    print("PLAY SOUND!");
                    soundProperties.soundInjector = Audio.playSound(soundProperties.sound, {
                        volume: soundProperties.volume,
                        position: newPosition,
                        loop: soundProperties.loop
                    });
                } else {
                    print ("RESTART INJECTOR!")
                    soundProperties.soundInjector.restart();
                }
                soundProperties.readyToPlay = false;
            } else if (soundProperties.sound && soundProperties.loop === false && soundProperties.interval !== -1) {
                // We need to check all of our entities that are not looping but have an interval associated with them
                // to see if it's time for them to play again
                soundProperties.timeSinceLastPlay += UPDATE_TIME;
                print("INCREMENT TIME SINCE LAST PLAY")
                if (soundProperties.timeSinceLastPlay > soundProperties.currentInterval) {
                    soundProperties.readyToPlay = true;
                    soundProperties.timeSinceLastPlay = 0;
                    // Now let's get our new current interval
                    soundProperties.currentInterval = soundProperties.interval + randFloat(-soundProperties.intervalSpread, soundProperties.intervalSpread);
                    soundProperties.currentInterval = Math.max(MIN_PLAY_INTERVAL, soundProperties.currentInterval);
                }
            }
        }
    }
}


function handleFoundSoundEntities(entities) {
    entities.forEach(function(entity) {
        var soundData = getEntityCustomData(SOUND_DATA_KEY, entity);
        if (soundData && soundData.url) {
            //check sound entities list- if it's not in, add it
            if (!soundEntityMap[entity]) {
                print("FOUND A NEW SOUND ENTITY!")
                var soundProperties = {
                    url: soundData.url,
                    volume: soundData.volume || DEFAULT_SOUND_DATA.volume,
                    loop: soundData.loop || DEFAULT_SOUND_DATA.loop,
                    interval: soundData.interval || DEFAULT_SOUND_DATA.interval,
                    intervalSpread: soundData.intervalSpread || DEFAULT_SOUND_DATA.intervalSpread,
                    readyToPlay: false,
                    position: Entities.getEntityProperties(entity, "position").position,
                    timeSinceLastPlay: 0,
                    timeWithoutAvatarInRange: 0,
                    isDownloaded: false
                };

                if (soundProperties.interval !== -1) {
                    soundProperties.currentInterval = soundProperties.interval + randFloat(-soundProperties.intervalSpread, soundProperties.intervalSpread);
                    soundProperties.currentInterval = Math.max(MIN_PLAY_INTERVAL, soundProperties.currentInterval);
                }

                soundEntityMap[entity] = soundProperties;
                if (!soundUrls[soundData.url]) {
                    // We need to download sound before we add it to our map
                    var sound = SoundCache.getSound(soundData.url);
                    // Only add it to map once it's downloaded
                    soundUrls[soundData.url] = sound;
                    sound.ready.connect(function() {
                        print("ADD TO MAP!")
                        soundProperties.sound = sound;
                        soundProperties.readyToPlay = true;
                        soundProperties.isDownloaded = true;
                        soundEntityMap[entity] = soundProperties;
                    });
                } else {
                    // We already have sound downloaded, so just add it to map right away
                    soundProperties.sound = soundUrls[soundData.url];
                    soundProperties.readyToPlay = true;
                    soundEntityMap[entity] = soundProperties;
                }
            } else {
                //If this sound is in our map already, we want to reset timeWithoutAvatarInRange
                // Also we want to check to see if the entity has been updated with new sound data- if so we want to update!
                soundEntityMap[entity].timeWithoutAvatarInRange = 0;
                checkForSoundPropertyChanges(soundEntityMap[entity], soundData);
            }
        }
    });
}

function checkForSoundPropertyChanges(currentProps, newProps) {
    var needsUpdate = false;
    if (currentProps.volume !== newProps.volume) {
        print("VOLUME CHANGED!!");
        currentProps.volume = newProps.volume;
        needsUpdate = true;
    }
    if (currentProps.url !== newProps.url) {
        currentProps.url = newProps.url;
        currentProps.sound = null;
        if (!soundUrls[currentProps.url]) {
            var sound = SoundCache.getSound(currentProps.url);
            currentProps.isDownloaded = false;
            sound.ready.connect(function() {
                print("Ready to play new sound!")
                currentProps.sound = sound;
                currentProps.isDownloaded = true;
            });
        } else {
            currentProps.sound = sound;
        }
        needsUpdate = true;
    }
    if (needsUpdate) {
        print("UPDATING!");
        // If we were looping we need to stop that so new changes are applied
        currentProps.soundInjector.stop();
        currentProps.soundInjector = null;
        currentProps.readyToPlay = true;
    }

}

Script.setInterval(slowUpdate, UPDATE_TIME);