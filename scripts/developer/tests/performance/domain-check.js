"use strict";
/*jslint vars: true, plusplus: true*/
/*globals Script, MyAvatar, Quat, Vec3, Render, ScriptDiscoveryService, Window, LODManager, Entities, Messages, AvatarList, Menu, Stats, HMD, location, print*/
//
//  loadedMachine.js
//  scripts/developer/tests/
//
//  Created by Howard Stearns on 6/6/16.
//  Copyright 2016 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
//  Confirms that the specified domain is operating within specified constraints.

var MINIMUM_DESKTOP_FRAMERATE = 57; // frames per second
var MINIMUM_HMD_FRAMERATE = 86;
var EXPECTED_DESKTOP_FRAMERATE = 60;
var EXPECTED_HMD_FRAMERATE = 90;
var NOMINAL_LOAD_TIME = 30;         // seconds
var MAXIMUM_LOAD_TIME = NOMINAL_LOAD_TIME * 2;
var MINIMUM_AVATARS = 25; // changeable by prompt

// If we add or remove things too quickly, we get problems (e.g., audio, fogbugz 2095).
// For now, spread them out this timing apart.
var SPREAD_TIME_MS = 500;

var DENSITY = 0.3; // square meters per person. Some say 10 sq ft is arm's length (0.9m^2), 4.5 is crowd (0.4m^2), 2.5 is mosh pit (0.2m^2).
var SOUND_DATA = {url: "http://hifi-content.s3.amazonaws.com/howard/sounds/piano1.wav"};
var AVATARS_CHATTERING_AT_ONCE = 4; // How many of the agents should we request to play SOUND at once.
var NEXT_SOUND_SPREAD = 500; // millisecond range of how long to wait after one sound finishes, before playing the next
var ANIMATION_DATA = {
    "url": "http://hifi-content.s3.amazonaws.com/howard/resources/avatar/animations/idle.fbx",
    // "url": "http://hifi-content.s3.amazonaws.com/howard/resources/avatar/animations/walk_fwd.fbx", // alternative example
    "startFrame": 0.0,
    "endFrame": 300.0,
    "timeScale": 1.0,
    "loopFlag": true
};

var version = 4;
function debug() {
    print.apply(null, [].concat.apply(['hrs fixme', version], [].map.call(arguments, JSON.stringify)));
}

function canonicalizePlacename(name) {
    var prefix = 'dev-';
    name = name.toLowerCase();
    if (name.indexOf(prefix) === 0) {
        name = name.slice(prefix.length);
    }
    return name;
}
var cachePlaces = ['localhost', 'welcome'].map(canonicalizePlacename); // For now, list the lighter weight one first.
var defaultPlace = location.hostname;
var prompt = "domain-check.js version " + version + "\n\nWhat place should we enter?";
debug(cachePlaces, defaultPlace, prompt);
var entryPlace = Window.prompt(prompt, defaultPlace);
var runTribbles = Window.confirm("Run tribbles?\n\n\
At most, only one participant should say yes.");
MINIMUM_AVATARS = parseInt(Window.prompt("Total avatars (including yourself and any already present)?", MINIMUM_AVATARS.toString()) || "0", 10);
AVATARS_CHATTERING_AT_ONCE = MINIMUM_AVATARS ? parseInt(Window.prompt("Number making sound?", Math.min(MINIMUM_AVATARS - 1, AVATARS_CHATTERING_AT_ONCE).toString()) || "0", 10) : 0;

function placesMatch(a, b) { // handling case and 'dev-' variations
    return canonicalizePlacename(a) === canonicalizePlacename(b);
}
function isNowIn(place) { // true if currently in specified place
    placesMatch(location.hostname, place);
}

function go(place) { // handle (dev-)welcome in the appropriate version-specific way
    debug('go', place);
    if (placesMatch(place, 'welcome')) {
        location.goToEntry();
    } else {
        location.handleLookupString(place);
    }
}

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
    var message = {}, avatarIdentifiers;
    try {
        message = JSON.parse(messageString);
    } catch (e) {
        print(e);
    }
    switch (message.key) {
    case "hello":
        Script.setTimeout(function () {
            // There can be avatars we've summoned that do not yet appear in the AvatarList.
            avatarIdentifiers = without(AvatarList.getAvatarIdentifiers(), summonedAgents);
            debug('present', avatarIdentifiers, summonedAgents);
            if ((summonedAgents.length + avatarIdentifiers.length) < MINIMUM_AVATARS) {
                var chatter = chattering.length < AVATARS_CHATTERING_AT_ONCE;
                if (chatter) {
                    chattering.push(senderID);
                }
                summonedAgents.push(senderID);
                messageSend({
                    key: 'SUMMON',
                    rcpt: senderID,
                    position: Vec3.sum(MyAvatar.position, {x: coord(), y: 0, z: coord()}),
                    orientation: Quat.fromPitchYawRollDegrees(0, Quat.safeEulerAngles(MyAvatar.orientation).y + (turnSpread * (Math.random() - 0.5)), 0),
                    soundData: chatter && SOUND_DATA,
                    listen: true,
                    skeletonModelURL: "http://hifi-content.s3.amazonaws.com/howard/resources/meshes/defaultAvatar_full.fst",
                    animationData: ANIMATION_DATA
                });
            }
        }, accumulatedDelay);
        accumulatedDelay += SPREAD_TIME_MS; // assume we'll get all the hello respsponses more or less together.
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
        print("crowd-agent received unrecognized message:", messageString);
    }
}
Messages.subscribe(MESSAGE_CHANNEL);
Messages.messageReceived.connect(messageHandler);
Script.scriptEnding.connect(function () {
    debug('stopping agents', summonedAgents);
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

var fail = false, results = "";
function addResult(label, actual, nominal, minimum, maximum) {
    if ((minimum !== undefined) && (actual < minimum)) {
        fail = ' FAILED: ' + label + ' below ' + minimum;
    }
    if ((maximum !== undefined) && (actual > maximum)) {
        fail = ' FAILED: ' + label + ' above ' + maximum;
    }
    results += "\n" + label + ": " + actual.toFixed(0) + " (" + ((100 * actual) / nominal).toFixed(0) + "%)";
}
function giveReport() {
    Window.alert(entryPlace + (fail || " OK") + "\n" + results + "\nwith " + summonedAgents.length + " avatars added,\nand " + AVATARS_CHATTERING_AT_ONCE + " making noise.");
}

// Tests are performed domain-wide, at full LOD
var initialLodIsAutomatic = LODManager.getAutomaticLODAdjust();
var LOD = 32768 * 400;
LODManager.setAutomaticLODAdjust(false);
LODManager.setOctreeSizeScale(LOD);
Script.scriptEnding.connect(function () { LODManager.setAutomaticLODAdjust(initialLodIsAutomatic); });

function startTwirl(targetRotation, degreesPerUpdate, interval, strafeDistance, optionalCallback) {
    var initialRotation = Quat.safeEulerAngles(MyAvatar.orientation).y;
    var accumulatedRotation = 0;
    function tick() {
        MyAvatar.orientation = Quat.fromPitchYawRollDegrees(0, accumulatedRotation + initialRotation, 0);
        if (strafeDistance) {
            MyAvatar.position = Vec3.sum(MyAvatar.position, Vec3.multiply(strafeDistance, Quat.getRight(MyAvatar.orientation)));
        }
        accumulatedRotation += degreesPerUpdate;
        if (accumulatedRotation >= targetRotation) {
            return optionalCallback && optionalCallback();
        }
        Script.setTimeout(tick, interval);
    }
    tick();
}

function doLoad(place, continuationWithLoadTime) { // Go to place and call continuationWithLoadTime(loadTimeInSeconds)
    var start = Date.now(), timeout, onDownloadUpdate, finishedTwirl = false, loadTime;
    // There are two ways to learn of changes: connect to change signals, or poll.
    // Until we get reliable results, we'll poll.
    var POLL_INTERVAL = 500, poll;
    function setHandlers() {
        //Stats.downloadsPendingChanged.connect(onDownloadUpdate); downloadsChanged, and physics...
        poll = Script.setInterval(onDownloadUpdate, POLL_INTERVAL);
    }
    function clearHandlers() {
        debug('clearHandlers');
        //Stats.downloadsPendingChanged.disconnect(onDownloadUpdate); downloadsChanged, and physics..
        Script.clearInterval(poll);
    }
    function waitForLoad(flag) {
        debug('entry', place, 'initial downloads/pending', Stats.downloads, Stats.downloadsPending);
        location.hostChanged.disconnect(waitForLoad);
        timeout = Script.setTimeout(function () {
            debug('downloads timeout', Date());
            clearHandlers();
            Window.alert("Timeout during " + place + " load. FAILED");
            Script.stop();
        }, MAXIMUM_LOAD_TIME * 1000);
        startTwirl(360, 6, 90, null, function () {
            finishedTwirl = true;
            if (loadTime) {
                continuationWithLoadTime(loadTime);
            }
        });
        setHandlers();
    }
    function isLoading() {
        // FIXME: We should also confirm that textures have loaded.
        return Stats.downloads || Stats.downloadsPending || !Window.isPhysicsEnabled();
    }
    onDownloadUpdate = function onDownloadUpdate() {
        debug('update downloads/pending', Stats.downloads, Stats.downloadsPending);
        if (isLoading()) {
            return;
        }
        Script.clearTimeout(timeout);
        clearHandlers();
        loadTime = (Date.now() - start) / 1000;
        if (finishedTwirl) {
            continuationWithLoadTime(loadTime);
        }
    };

    location.hostChanged.connect(waitForLoad);
    go(place);
}

var config = Render.getConfig("Stats");
function doRender(continuation) {
    var start = Date.now(), frames = 0;
    function onNewStats() { // Accumulates frames on signal during load test
        frames++;
    }
    if (MINIMUM_AVATARS) {
        messageSend({key: 'HELO'}); // Ask agents to report in now.
    }

    config.newStats.connect(onNewStats);
    startTwirl(720, 1, 20, 0.08, function () {
        var end = Date.now();
        config.newStats.disconnect(onNewStats);
        addResult('frame rate', 1000 * frames / (end - start),
                  HMD.active ? EXPECTED_HMD_FRAMERATE : EXPECTED_DESKTOP_FRAMERATE,
                  HMD.active ? MINIMUM_HMD_FRAMERATE : MINIMUM_DESKTOP_FRAMERATE);
        var total = AvatarList.getAvatarIdentifiers().length;
        if (MINIMUM_AVATARS && !fail) {
            if (0 === summonedAgents.length) {
                fail = "FAIL: No agents reported.\nPlease run " + MINIMUM_AVATARS + " instances of\n\
http://hifi-content.s3.amazonaws.com/howard/scripts/tests/performance/crowd-agent.js?v=3\n\
on your domain server.";
            } else if (total < MINIMUM_AVATARS) {
                fail = "FAIL: Only " + summonedAgents.length + " agents reported. Now missing " + (MINIMUM_AVATARS - total) + " avatars, total.";
            }
        }
        continuation();
    });
}

var TELEPORT_PAUSE = 500;
function prepareCache(continuation) {
    function loadNext() {
        var place = cachePlaces.shift();
        doLoad(place, function (prepTime) {
            debug(place, 'ready', prepTime);
            if (cachePlaces.length) {
                loadNext();
            } else {
                continuation();
            }
        });
    }
    // remove entryPlace target from cachePlaces
    var targetInCache = cachePlaces.indexOf(canonicalizePlacename(entryPlace));
    if (targetInCache !== -1) {
        cachePlaces.splice(targetInCache, 1);
    }
    debug('cachePlaces', cachePlaces);
    go(cachePlaces[1] || entryPlace); // Not quite right for entryPlace case (allows some qt pre-caching), but close enough.
    Script.setTimeout(function () {
        Menu.triggerOption("Reload Content (Clears all caches)");
        Script.setTimeout(loadNext, TELEPORT_PAUSE);
    }, TELEPORT_PAUSE);
}

function maybeRunTribbles(continuation) {
    if (runTribbles) {
        Script.load('http://cdn.highfidelity.com/davidkelly/production/scripts/tests/performance/tribbles.js');
        Script.setTimeout(continuation, 3000);
    } else {
        continuation();
    }
}

if (!entryPlace) {
    Window.alert("domain-check.js cancelled");
    Script.stop();
} else {
    prepareCache(function (prepTime) {
        debug('cache ready', prepTime);
        doLoad(entryPlace, function (loadTime) {
            addResult("load time", loadTime, NOMINAL_LOAD_TIME, undefined, MAXIMUM_LOAD_TIME);
            LODManager.setAutomaticLODAdjust(initialLodIsAutomatic); // after loading, restore lod.
            maybeRunTribbles(function () {
                doRender(function () {
                    giveReport();
                    Script.stop();
                });
            });
        });
    });
}

Script.scriptEnding.connect(function () { print("domain-check completed"); });
