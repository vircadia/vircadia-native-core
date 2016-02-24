"use strict";
/*jslint nomen: true, plusplus: true, vars: true*/
/*global AvatarList, Entities, EntityViewer, Script, SoundCache, Audio, print, randFloat*/
//
//  ACAudioSearchAndInject.js
//  audio
//
//  Created by Eric Levin and Howard Stearns 2/1/2016
//  Copyright 2016 High Fidelity, Inc.
//
//  Keeps track of all sounds within QUERY_RADIUS of an avatar, where a "sound" is specified in entity userData.
//  Inject as many as practical into the audio mixer.
//  See acAudioSearchAndCompatibilityEntitySpawner.js.
//
//  This implementation takes some precautions to scale well:
//  - It doesn't hastle the entity server because it issues at most one octree query every UPDATE_TIME period, regardless of the number of avatars.
//  - It does not load itself because it only gathers entities once every UPDATE_TIME period, and only
//    checks entity properties for those small number of entities that are currently playing (plus a RECHECK_TIME period examination of all entities).
//  This implementation tries to use all the available injectors.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html

var SOUND_DATA_KEY = "io.highfidelity.soundKey"; // Sound data is specified in userData under this key.
var old_sound_data_key = "soundKey"; // For backwards compatibility.
var QUERY_RADIUS = 50; // meters
var UPDATE_TIME = 100; // ms. We'll update just one thing on this period.
var EXPIRATION_TIME = 5 * 1000; // ms. Remove sounds that have been out of range for this time.
var RECHECK_TIME = 10 * 1000; // ms. Check for new userData properties this often when not currently playing.
// (By not checking most of the time when not playing, we can efficiently go through all entities without getEntityProperties.)
var UPDATES_PER_STATS_LOG = 50;

var DEFAULT_SOUND_DATA = {
    volume: 0.5, // userData cannot specify zero volume with our current method of defaulting.
    loop: false, // Default must be false with our current method of defaulting, else there's no way to get a false value.
    playbackGap: 1000, // in ms
    playbackGapRange: 0 // in ms
};

Script.include("../../libraries/utils.js");
Agent.isAvatar = true; // This puts a robot at 0,0,0, but is currently necessary in order to use AvatarList.
function ignore() {}
function debug() { // Display the arguments not just [Object object].
    //print.apply(null, [].map.call(arguments, JSON.stringify));
}

EntityViewer.setKeyholeRadius(QUERY_RADIUS);

// ENTITY DATA CACHE
// 
var entityCache = {}; // A dictionary of unexpired EntityData objects.
function EntityDatum(entityIdentifier) { // Just the data of an entity that we need to know about.
    // This data is only use for our sound injection. There is no need to store such info in the replicated entity on everyone's computer.
    var that = this;
    that.lastUserDataUpdate = 0; // new entity is in need of rechecking user data
    // State Transitions:
    // no data => no data | sound data | expired
    //   expired => stop => remove
    //   sound data => downloading
    //     downloading => downloading | waiting
    //       waiting => playing | waiting (if too many already playing)
    //         playing => update position etc | no data
    that.stop = function stop() {
        if (!that.sound) {
            return;
        }
        print("stopping sound", entityIdentifier, that.url);
        delete that.sound;
        delete that.url;
        if (!that.injector) {
            return;
        }
        that.injector.stop();
        delete that.injector;
    };
    this.update = function stateTransitions(expirationCutoff, userDataCutoff, now) {
        if (that.timestamp < expirationCutoff) {                             // EXPIRED => STOP => REMOVE
            that.stop(); // Alternatively, we could fade out and then stop...
            delete entityCache[entityIdentifier];
            return;
        }
        var properties, soundData; // Latest data, pulled from local octree.
        // getEntityProperties locks the tree, which competes with the asynchronous processing of queryOctree results.
        // Most entity updates are fast and only a very few do getEntityProperties.
        function ensureSoundData() { // We only getEntityProperities when we need to.
            if (properties) {
                return;
            }
            properties = Entities.getEntityProperties(entityIdentifier, ['userData', 'position']);
            debug("updating", that, properties);
            try {
                var userData = properties.userData && JSON.parse(properties.userData);
                soundData = userData && (userData[SOUND_DATA_KEY] || userData[old_sound_data_key]); // Don't store soundData yet. Let state changes compare.
                that.lastUserDataUpdate = now;                    // But do update these ...
                that.url = soundData && soundData.url;
                that.playAfter = that.url && now;
            } catch (err) {
                print(err, properties.userData);
            }
        }
        // Stumbling on big new pile of entities will do a lot of getEntityProperties. Once.
        if (that.lastUserDataUpdate < userDataCutoff) {                      // NO DATA => SOUND DATA
            ensureSoundData();
        }
        if (!that.url) {                                                     // NO DATA => NO DATA
            return that.stop();
        }
        if (!that.sound) {                                                   // SOUND DATA => DOWNLOADING
            that.sound = SoundCache.getSound(soundData.url); // SoundCache can manage duplicates better than we can.
        }
        if (!that.sound.downloaded) {                                        // DOWNLOADING => DOWNLOADING
            return;
        }
        if (that.playAfter > now) {                                          // DOWNLOADING | WAITING => WAITING
            return;
        }
        ensureSoundData(); // We'll play and will need position, so we might as well get soundData, too.
        if (soundData.url !== that.url) {                                    // WAITING => NO DATA (update next time around)
            return that.stop();
        }
        var options = {
            position: properties.position,
            loop: soundData.loop || DEFAULT_SOUND_DATA.loop,
            volume: soundData.volume || DEFAULT_SOUND_DATA.volume
        };
        if (!that.injector) {                                                // WAITING => PLAYING | WAITING
            debug("starting", that, options);
            that.injector = Audio.playSound(that.sound, options); // Might be null if at at injector limit. Will try again later.
            if (that.injector) {
                print("started", entityIdentifier, that.url);
            } else { // Don't hammer ensureSoundData or injector manager.
                that.playAfter = now + (soundData.playbackGap || RECHECK_TIME);
            }
            return;
        }
        that.injector.setOptions(options);                                   // PLAYING => UPDATE POSITION ETC
        if (!that.injector.isPlaying) { // Subtle: a looping sound will not check playbackGap.
            var gap = soundData.playbackGap || DEFAULT_SOUND_DATA;
            if (gap) {                                                       // WAITING => PLAYING
                gap = gap + randFloat(-Math.max(gap, soundData.playbackGapRange), soundData.playbackGapRange); // gapRange is bad name. Meant as +/- value.
                // Setup next play just once, now. Changes won't be looked at while we wait.
                that.playAfter = now + (that.sound.duration * 1000) + gap;
                // Subtle: if the restart fails b/c we're at injector limit, we won't try again until next playAfter.
                that.injector.restart();
            } else {                                                         // PLAYING => NO DATA
                that.playAfter = Infinity;
            }
        }
    };
}
function internEntityDatum(entityIdentifier, timestamp, avatarPosition, avatar) {
    ignore(avatarPosition, avatar); // We could use avatars and/or avatarPositions to prioritize which ones to play.
    var entitySound = entityCache[entityIdentifier];
    if (!entitySound) {
        entitySound = entityCache[entityIdentifier] = new EntityDatum(entityIdentifier);
    }
    entitySound.timestamp = timestamp; // Might be updated for multiple avatars. That's fine.
}
var nUpdates = UPDATES_PER_STATS_LOG, lastStats = Date.now();
function updateAllEntityData() { // A fast update of all entities we know about. A few make sounds.
    var now = Date.now(),
        expirationCutoff = now - EXPIRATION_TIME,
        userDataRecheckCutoff = now - RECHECK_TIME;
    Object.keys(entityCache).forEach(function (entityIdentifier) {
        entityCache[entityIdentifier].update(expirationCutoff, userDataRecheckCutoff, now);
    });
    if (nUpdates-- <= 0) { // Report statistics.
        // My figures using acAudioSearchCompatibleEntitySpawner.js with ONE user, N_SOUNDS = 2000, N_SILENT_ENTITIES_PER_SOUND = 5:
        // audio-mixer: 23% of cpu (on Mac Activity Monitor)
        // this script's assignment client: 106% of cpu. (overloaded)
        // entities:12003
        // sounds:2000
        // playing:40 (correct)
        // millisecondsPerUpdate:135 (100 requested, so behind by 35%. It would be nice to dig into why...)
        var stats = {entities: 0, sounds: 0, playing: 0, millisecondsPerUpdate: (now - lastStats) / UPDATES_PER_STATS_LOG};
        nUpdates = UPDATES_PER_STATS_LOG;
        lastStats = now;
        Object.keys(entityCache).forEach(function (entityIdentifier) {
            var datum = entityCache[entityIdentifier];
            stats.entities++;
            if (datum.url) {
                stats.sounds++;
                if (datum.injector && datum.injector.isPlaying) {
                    stats.playing++;
                }
            }
        });
        print(JSON.stringify(stats));
    }
}

// Update the set of which EntityData we know about.
//
function updateEntiesForAvatar(avatarIdentifier) { // Just one piece of update work.
    // This does at most:
    //   one queryOctree request of the entity server, and
    //   one findEntities geometry query of our own octree, and
    //   a quick internEntityDatum of each of what may be a large number of entityIdentifiers.
    // The idea is that this is a nice bounded piece of work that should not be done too frequently.
    // However, it means that we won't learn about new entities until, on average (nAvatars * UPDATE_TIME) + query round trip.
    var avatar = AvatarList.getAvatar(avatarIdentifier), avatarPosition = avatar && avatar.position;
    if (!avatarPosition) { // No longer here.
        return;
    }
    var timestamp = Date.now();
    EntityViewer.setPosition(avatarPosition);
    EntityViewer.queryOctree(); // Requests an update, but there's no telling when we'll actually see different results.
    var entities = Entities.findEntities(avatarPosition, QUERY_RADIUS);
    debug("found", entities.length, "entities near", avatar.name || "unknown", "at", avatarPosition);
    entities.forEach(function (entityIdentifier) {
        internEntityDatum(entityIdentifier, timestamp, avatarPosition, avatar);
    });
}

// Slowly update the set of data we have to work with.
//
var workQueue = [];
function updateWorkQueueForAvatarsPresent() { // when nothing else to do, fill queue with individual avatar updates
    workQueue = AvatarList.getAvatarIdentifiers().map(function (avatarIdentifier) {
        return function () {
            updateEntiesForAvatar(avatarIdentifier);
        };
    });
}
Script.setInterval(function () {
    // There might be thousands of EntityData known to us, but only a few will require any work to update.
    updateAllEntityData(); // i.e., this better be pretty fast.
    // Each interval, we do no more than one updateEntitiesforAvatar.
    if (!workQueue.length) {
        workQueue = [updateWorkQueueForAvatarsPresent];
    }
    workQueue.pop()(); // There's always one
}, UPDATE_TIME);
