"use strict";
/*jslint vars: true, plusplus: true*/
/*globals Script, MyAvatar, Quat, Render, ScriptDiscoveryService, Window, LODManager, Entities, print*/
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
var MAXIMUM_LOAD_TIME = 60;         // seconds
var MINIMUM_AVATARS = 25;             // FIXME: not implemented yet. Requires agent scripts. Idea is to have them organize themselves to the right number.

var version = 1;
function debug() {
    print.apply(null, [].concat.apply(['hrs fixme', version], [].map.call(arguments, JSON.stringify)));
}

function isNowIn(place) { // true if currently in specified place
    return location.hostname.toLowerCase() === place.toLowerCase();
}

var cachePlaces = ['dev-Welcome', 'localhost']; // For now, list the lighter weight one first.
var isInCachePlace = cachePlaces.some(isNowIn);
var defaultPlace = isInCachePlace ? 'dev-Playa' : location.hostname;
var prompt = "domain-check.js version " + version + "\n\nWhat place should we enter?";
debug(cachePlaces, isInCachePlace, defaultPlace, prompt);
var entryPlace = Window.prompt(prompt, defaultPlace);

var fail = false, results = "";
function addResult(label, actual, minimum, maximum) {
    if ((minimum !== undefined) && (actual < minimum)) {
        fail = true;
    }
    if ((maximum !== undefined) && (actual > maximum)) {
        fail = true;
    }
    results += "\n" + label + ": " + actual + " (" + ((100 * actual) / (maximum || minimum)).toFixed(0) + "%)";
}
function giveReport() {
    Window.alert(entryPlace + (fail ? " FAILED" : " OK") + "\n" + results);
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
    function clearHandlers() {
        debug('clearHandlers');
        Stats.downloadsPendingChanged.disconnect(onDownloadUpdate);
        Stats.downloadsChanged.disconnect(onDownloadUpdate);
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
        Stats.downloadsPendingChanged.connect(onDownloadUpdate);
        Stats.downloadsChanged.connect(onDownloadUpdate);
    }
    function isLoading() {
        // FIXME: This tells us when download are completed, but it doesn't tell us when the objects are parsed and loaded.
        // We really want something like _physicsEnabled, but that isn't signalled.
        return Stats.downloads || Stats.downloadsPending;
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

    debug('go', place);
    location.hostChanged.connect(waitForLoad);
    location.handleLookupString(place);
}

var config = Render.getConfig("Stats");
function doRender(continuation) {
    var start = Date.now(), frames = 0;
    function onNewStats() { // Accumulates frames on signal during load test
        frames++;
    }
    config.newStats.connect(onNewStats);
    startTwirl(720, 1, 15, 0.08, function () {
        var end = Date.now();
        config.newStats.disconnect(onNewStats);
        addResult('frame rate', 1000 * frames / (end - start),
                  HMD.active ? MINIMUM_HMD_FRAMERATE : MINIMUM_DESKTOP_FRAMERATE,
                  HMD.active ? EXPECTED_HMD_FRAMERATE : EXPECTED_DESKTOP_FRAMERATE);
        continuation();
    });
}

var TELEPORT_PAUSE = 500;
function maybePrepareCache(continuation) {
    var prepareCache = Window.confirm("Prepare cache?\n\n\
Should we start with all and only those items cached that are encountered when visiting:\n" + cachePlaces.join(', ') + "\n\
If 'yes', cache will be cleared and we will visit these two, with a turn in each, and wait for everything to be loaded.\n\
You would want to say 'no' (and make other preparations) if you were testing these places.");

    if (prepareCache) {
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
        location.handleLookupString(cachePlaces[cachePlaces.length - 1]);
        Menu.triggerOption("Reload Content (Clears all caches)");
        Script.setTimeout(loadNext, TELEPORT_PAUSE);
    } else {
        location.handleLookupString(isNowIn(cachePlaces[0]) ? cachePlaces[1] : cachePlaces[0]);
        Script.setTimeout(continuation, TELEPORT_PAUSE);
    }
}

function maybeRunTribbles(continuation) {
    if (Window.confirm("Run tribbles?\n\n\
At most, only one participant should say yes.")) {
        Script.load('http://cdn.highfidelity.com/davidkelly/production/scripts/tests/performance/tribbles.js'); // FIXME: replace with AWS
        Script.setTimeout(continuation, 3000);
    } else {
        continuation();
    }
}

if (!entryPlace) {
    Window.alert("domain-check.js cancelled");
    Script.stop();
} else {
    maybePrepareCache(function (prepTime) {
        debug('cache ready', prepTime);
        doLoad(entryPlace, function (loadTime) {
            addResult("load time", loadTime, undefined, MAXIMUM_LOAD_TIME);
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
