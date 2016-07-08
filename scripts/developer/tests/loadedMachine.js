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
//  Characterises the performance of heavily loaded or struggling machines.
//  There is no point in running this on a machine that producing the target 60 or 90 hz rendering rate.
//
//  The script examines several source of load, including each running script and a couple of Entity types.
//  It turns all of these off, as well as LOD management, and twirls in place to get a baseline render rate.
//  Then it turns each load on, one at a time, and records a new render rate.
//  At the end, it reports the ordered results (in a popup), restores the original loads and LOD management, and quits.
//
//  Small performance changes are not meaningful.
//  If you think you find something that is significant, you should probably run again to make sure that it isn't random variation.
//  You can also compare (e.g., the baseline) for different conditions such as domain, version, server settings, etc.
//  This does not profile scripts as they are used -- it merely measures the effect of having the script loaded and running quietly.

var NUMBER_OF_TWIRLS_PER_LOAD = 10;
var MILLISECONDS_PER_SECOND = 1000;
var WAIT_TIME_MILLISECONDS = MILLISECONDS_PER_SECOND;
var accumulatedRotation = 0;
var start;
var frames = 0;
var config = Render.getConfig("Stats");
var thisPath = Script.resolvePath('');
var scriptData = ScriptDiscoveryService.getRunning();
var scripts = scriptData.filter(function (datum) { return datum.name !== 'defaultScripts.js'; }).map(function (script) { return script.path; });
// If defaultScripts.js is running, we leave it running, because restarting it safely is a mess.
var otherScripts = scripts.filter(function (path) { return path !== thisPath; });
var numberLeftRunning = scriptData.length - otherScripts.length;
print('initially running', otherScripts.length, 'scripts. Leaving', numberLeftRunning, 'and stopping', otherScripts);
var typedEntities = {Light: [], ParticleEffect: []};
var interestingTypes = Object.keys(typedEntities);
var propertiedEntities = {dynamic: []};
var interestingProperties = Object.keys(propertiedEntities);
var loads = ['ignore', 'baseline'].concat(otherScripts, interestingTypes, interestingProperties);
var loadIndex = 0, nLoads = loads.length, load;
var results = [];
var initialLodIsAutomatic = LODManager.getAutomaticLODAdjust();
var SEARCH_RADIUS = 17000;
var DEFAULT_LOD = 32768 * 400;
LODManager.setAutomaticLODAdjust(false);
LODManager.setOctreeSizeScale(DEFAULT_LOD);

// Fill the typedEnties with the entityIDs that are already visible. It would be nice if this were more efficient.
var allEntities = Entities.findEntities(MyAvatar.position, SEARCH_RADIUS);
print('Searching', allEntities.length, 'entities for', interestingTypes, 'and', interestingProperties);
var propertiesToGet = ['type', 'visible'].concat(interestingProperties);
allEntities.forEach(function (entityID) {
    var properties = Entities.getEntityProperties(entityID, propertiesToGet);
    if (properties.visible) {
        if (interestingTypes.indexOf(properties.type) >= 0) {
            typedEntities[properties.type].push(entityID);
        } else {
            interestingProperties.forEach(function (property) {
                if (entityID && properties[property]) {
                    propertiedEntities[property].push(entityID);
                    entityID = false; // Put in only one bin
                }
            });
        }
    }
});
allEntities = undefined; // free them
interestingTypes.forEach(function (type) {
    print('There are', typedEntities[type].length, type, 'entities.');
});
interestingProperties.forEach(function (property) {
    print('There are', propertiedEntities[property].length, property, 'entities.');
});
function toggleVisibility(type, on) {
    typedEntities[type].forEach(function (entityID) {
        Entities.editEntity(entityID, {visible: on});
    });
}
function toggleProperty(property, on) {
    propertiedEntities[property].forEach(function (entityID) {
        var properties = {};
        properties[property] = on;
        Entities.editEntity(entityID, properties);
    });
}
function restoreOneTest(load) {
    print('restore', load);
    switch (load) {
    case 'baseline':
    case 'ignore':  // ignore is used to do a twirl to make sure everything is loaded.
        break;
    case 'Light':
    case 'ParticleEffect':
        toggleVisibility(load, true);
        break;
    case 'dynamic':
        toggleProperty(load, 1);
        break;
    default:
        Script.load(load);
    }
}
function undoOneTest(load) {
    print('stop', load);
    switch (load) {
    case 'baseline':
    case 'ignore':
        break;
    case 'Light':
    case 'ParticleEffect':
        toggleVisibility(load, false);
        break;
    case 'dynamic':
        toggleProperty(load, 0);
        break;
    default:
        ScriptDiscoveryService.stopScript(load);
    }
}
loads.forEach(undoOneTest);

function finalReport() {
    var baseline = results[0];
    results.forEach(function (result) {
        result.ratio = (baseline.fps / result.fps);
    });
    results.sort(function (a, b) { return b.ratio - a.ratio; });
    var report = 'Performance Report:\nBaseline: ' + baseline.fps.toFixed(1) + ' fps.';
    results.forEach(function (result) {
        report += '\n' + result.ratio.toFixed(2) + ' (' + result.fps.toFixed(1) + ' fps over ' + result.elapsed + ' seconds) for ' + result.name;
    });
    Window.alert(report);
    LODManager.setAutomaticLODAdjust(initialLodIsAutomatic);
    loads.forEach(restoreOneTest);
    Script.stop();
}

function onNewStats() { // Accumulates frames on signal during load test
    frames++;
}
var DEGREES_PER_FULL_TWIRL = 360;
var DEGREES_PER_UPDATE = 6;
function onUpdate() { // Spins on update signal during load test
    MyAvatar.orientation = Quat.fromPitchYawRollDegrees(0, accumulatedRotation, 0);
    accumulatedRotation += DEGREES_PER_UPDATE;
    if (accumulatedRotation >= (DEGREES_PER_FULL_TWIRL * NUMBER_OF_TWIRLS_PER_LOAD)) {
        tearDown();
    }
}
function startTest() {
    print('start', load);
    accumulatedRotation = frames = 0;
    start = Date.now();
    Script.update.connect(onUpdate);
    config.newStats.connect(onNewStats);
}

function setup() {
    if (loadIndex >= nLoads) {
        return finalReport();
    }
    load = loads[loadIndex];
    restoreOneTest(load);
    Script.setTimeout(startTest, WAIT_TIME_MILLISECONDS);
}
function tearDown() {
    var now = Date.now();
    var elapsed = (now - start) / MILLISECONDS_PER_SECOND;
    Script.update.disconnect(onUpdate);
    config.newStats.disconnect(onNewStats);
    if (load !== 'ignore') {
        results.push({name: load, fps: frames / elapsed, elapsed: elapsed});
    }
    undoOneTest(load);
    loadIndex++;
    setup();
}
function waitUntilStopped() { // Wait until all the scripts have stopped
    var count = ScriptDiscoveryService.getRunning().length;
    if (count === numberLeftRunning) {
        return setup();
    }
    print('Still', count, 'scripts running');
    Script.setTimeout(waitUntilStopped, WAIT_TIME_MILLISECONDS);
}
waitUntilStopped();
