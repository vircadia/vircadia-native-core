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

var MSEC_PER_SEC = 1000;
var SOUND_DATA_KEY = "io.highfidelity.soundKey"; // Sound data is specified in userData under this key.
var old_sound_data_key = "soundKey"; // For backwards compatibility.
var QUERY_RADIUS = 50; // meters
var UPDATE_TIME = 100; // ms. We'll update just one thing on this period.
var EXPIRATION_TIME = 5 * MSEC_PER_SEC; // ms. Remove sounds that have been out of range for this time.
var RECHECK_TIME = 10 * MSEC_PER_SEC; // ms. Check for new userData properties this often when not currently playing.
// (By not checking most of the time when not playing, we can efficiently go through all entities without getEntityProperties.)
var UPDATES_PER_STATS_LOG = RECHECK_TIME / UPDATE_TIME; // (It's nice to smooth out the results by straddling a recheck.)

var DEFAULT_SOUND_DATA = {
    volume: 0.5, // userData cannot specify zero volume with our current method of defaulting.
    loop: false, // Default must be false with our current method of defaulting, else there's no way to get a false value.
    playbackGap: MSEC_PER_SEC, // in ms
    playbackGapRange: 0 // in ms
};

//var isACScript = this.EntityViewer !== undefined;
var isACScript = true;

Script.include("http://hifi-content.s3.amazonaws.com/ryan/development/utils_ryan.js");
if (isACScript) {
    Agent.isAvatar = true; // This puts a robot at 0,0,0, but is currently necessary in order to use AvatarList.
    Avatar.skeletonModelURL = "http://hifi-content.s3.amazonaws.com/ozan/dev/avatars/invisible_avatar/invisible_avatar.fst";
}
function ignore() {}
function debug() { // Display the arguments not just [Object object].
    //print.apply(null, [].map.call(arguments, JSON.stringify));
}

if (isACScript) {
    EntityViewer.setCenterRadius(QUERY_RADIUS);
}

// ENTITY DATA CACHE
//
var entityCache = {}; // A dictionary of unexpired EntityData objects.
var entityInvalidUserDataCache = {}; // A cache containing the entity IDs that have
                                     // previously been identified as containing non-JSON userData.
                                     // We use a dictionary here so id lookups are constant time.
var examinationCount = 0;
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
            examinationCount++; // Collect statistics on how many getEntityProperties we do.
            debug("updating", that, properties);
            try {
                var userData = properties.userData && JSON.parse(properties.userData);
                soundData = userData && (userData[SOUND_DATA_KEY] || userData[old_sound_data_key]); // Don't store soundData yet. Let state changes compare.
                that.lastUserDataUpdate = now;                    // But do update these ...
                that.url = soundData && soundData.url;
                that.playAfter = that.url && now;
            } catch (err) {
                if (!(entityIdentifier in entityInvalidUserDataCache)) {
                    print(err, properties.userData);
                    entityInvalidUserDataCache[entityIdentifier] = true;
                }
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
        ensureSoundData(); // We'll try to play/setOptions and will need position, so we might as well get soundData, too.
        if (soundData.url !== that.url) {                                    // WAITING => NO DATA (update next time around)
            return that.stop();
        }
        var options = {
            position: properties.position,
            loop: soundData.loop || DEFAULT_SOUND_DATA.loop,
            volume: soundData.volume || DEFAULT_SOUND_DATA.volume
        };
        function repeat() {
            return !options.loop && (soundData.playbackGap >= 0);
        }
        function randomizedNextPlay() { // time of next play or recheck, randomized to distribute the work
            var range = soundData.playbackGapRange || DEFAULT_SOUND_DATA.playbackGapRange,
                base = repeat() ? ((that.sound.duration * MSEC_PER_SEC) + (soundData.playbackGap || DEFAULT_SOUND_DATA.playbackGap)) : RECHECK_TIME;
            return now + base + randFloat(-Math.min(base, range), range);
        }
        if (that.injector && soundData.playing === false) {
            that.injector.stop();
            that.injector = null;
        }
        if (!that.injector) {
            if (soundData.playing === false) {                                                // WAITING => PLAYING | WAITING
                return;
            }
            debug("starting", that, options);
            that.injector = Audio.playSound(that.sound, options); // Might be null if at at injector limit. Will try again later.
            if (that.injector) {
                print("started", entityIdentifier, that.url);
            } else { // Don't hammer ensureSoundData or injector manager.
                that.playAfter = randomizedNextPlay();
            }
            return;
        }
        that.injector.setOptions(options);                                   // PLAYING => UPDATE POSITION ETC
        if (!that.injector.playing) { // Subtle: a looping sound will not check playbackGap.
            if (repeat()) {                                                  // WAITING => PLAYING
                // Setup next play just once, now. Changes won't be looked at while we wait.
                that.playAfter = randomizedNextPlay();
                // Subtle: if the restart fails b/c we're at injector limit, we won't try again until next playAfter.
                that.injector.restart();
            } else {                                                        // PLAYING => NO DATA
                that.playAfter = Infinity; // was one-shot and we're finished
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
        // For example, with:
        //  injector-limit = 40 (in C++ code)
        //  N_SOUNDS = 1000    (from userData in, e.g., acAudioSearchCompatibleEntitySpawner.js)
        //  replay-period = 3 + 20 = 23 (seconds, ditto)
        //  stats-period = UPDATES_PER_STATS_LOG * UPDATE_TIME / MSEC_PER_SEC = 10 seconds
        // The log should show between each stats report:
        //  "start" lines ~= injector-limit * P(finish) = injector-limit * stats-period/replay-period = 17 ?
        //  total attempts at starting ("start" lines + "could not thread" lines) ~= N_SOUNDS = 1000 ?
        //  entities > N_SOUNDS * (1+ N_SILENT_ENTITIES_PER_SOUND) = 11000 + whatever was in the scene before running spawner
        //  sounds = N_SOUNDS = 1000
        //  getEntityPropertiesPerUpdate ~= playing + failed-starts/UPDATES_PER_STATS_LOG + other-rechecks-each-update
        //     = injector-limit + (total attempts - "start" lines)/UPDATES_PER_STATS__LOG
        //       + (entities - playing - failed-starts/UPDATES_PER_STATS_LOG) * P(recheck-in-update)
        //                     where failed-starts/UPDATES_PER_STATS_LOG = (1000-17)/100 = 10
        //     = 40 + 10 + (11000 - 40 - 10)*UPDATE_TIME/RECHECK_TIME
        //     = 40 + 10 + 10950*0.01 = 159   (mostly proportional to enties/RECHECK_TIME)
        //  millisecondsPerUpdate ~= UPDATE_TIME = 100 (+ some timer machinery time)
        //  this assignment client activity monitor < 100% cpu
        var stats = {
            entities: 0,
            sounds: 0,
            playing: 0,
            getEntityPropertiesPerUpdate: examinationCount / UPDATES_PER_STATS_LOG,
            millisecondsPerUpdate: (now - lastStats) / UPDATES_PER_STATS_LOG
        };
        nUpdates = UPDATES_PER_STATS_LOG;
        lastStats = now;
        examinationCount = 0;
        Object.keys(entityCache).forEach(function (entityIdentifier) {
            var datum = entityCache[entityIdentifier];
            stats.entities++;
            if (datum.url) {
                stats.sounds++;
                if (datum.injector && datum.injector.playing) {
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
    if (isACScript) {
        EntityViewer.setPosition(avatarPosition);
        EntityViewer.queryOctree(); // Requests an update, but there's no telling when we'll actually see different results.
    }
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
