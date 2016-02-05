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

var UPDATE_TIME = 3000;
var EXPIRATION_TIME = 5000; 

var soundEntityMap = {};

print("EBL STARTING SCRIPT");

function slowUpdate() {
    var avatars = AvatarList.getAvatarIdentifiers();
    for (var i = 0; i < avatars.length; i++) {
        var avatar = AvatarList.getAvatar(avatars[i]);
        EntityViewer.setPosition(avatar.position);
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
        if (soundProperties.timeWithoutAvatarInRange > EXPIRATION_TIME) {
            // An avatar hasn't been within range of this sound entity recently, so remove it from map
            print ("NO AVATARS HAVE BEEN AROUND FOR A WHILE SO REMOVE THIS SOUND FROM SOUNDMAP!");
            delete soundEntityMap[soundEntity];
            print("NEW MAP " + JSON.stringify(soundEntityMap));
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
                    readyToPlay: true,
                    position: Entities.getEntityProperties(entity, "position").position,
                    timeSinceLastPlay: 0,
                    timeWithoutAvatarInRange: 0
                };

                // If it's not in list, add it
                soundEntityMap[entity] = soundProperties;
            } else {
                //If this sound is in our map already, we want to reset timeWithoutAvatarInRange 
                soundEntityMap[entity].timeWithoutAvatarInRange = 0;
            }
        }
    });
}
Script.setInterval(slowUpdate, UPDATE_TIME);