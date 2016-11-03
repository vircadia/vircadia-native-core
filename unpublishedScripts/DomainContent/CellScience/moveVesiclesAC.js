//  Copyright 2016 High Fidelity, Inc.
//
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
var WORLD_OFFSET = {
    x: 0,
    y: 0,
    z: 0
}

var WORLD_SCALE_AMOUNT = 1.0;

function offsetVectorToWorld(vector) {
    var newVector;

    newVector = Vec3.sum(vector, WORLD_OFFSET);

    return newVector
}

var basePosition = offsetVectorToWorld({
    x: 3000,
    y: 13500,
    z: 3000
}, WORLD_OFFSET);


var initialized = false;

EntityViewer.setPosition(basePosition);
EntityViewer.setKeyholeRadius(60000);
var octreeQueryInterval = Script.setInterval(function() {
    EntityViewer.queryOctree();
}, 200);

var THROTTLE = true;
var THROTTLE_RATE = 5000;

var sinceLastUpdate = 0;
var entitiesToMove = [];

//print('vesicle script')

function findVesicles() {
    var results = Entities.findEntities(basePosition, 60000);
    Script.clearInterval(octreeQueryInterval); // we don't need it any more

    if (results.length === 0) {
        // print('no entities found');
        return;
    }

    results.forEach(function(v) {
        var name = Entities.getEntityProperties(v, 'name').name;
        if (name === 'vesicle') {
            //print('found a vesicle!!' + v)
            entitiesToMove.push(v);
        }
    });
}

var minVelocity = 1;
var maxVelocity = 5;
var minAngularVelocity = 0.01;
var maxAngularVelocity = 0.03;

function moveVesicle(entityId) {
    // print('moving a vesicle! ' + entityId)
    var magnitudeV = maxVelocity;
    var directionV = {
        x: Math.random() - 0.5,
        y: Math.random() - 0.5,
        z: Math.random() - 0.5
    };

    // print("POS magnitude is " + magnitudeV + " and direction is " + directionV.x);

    var magnitudeAV = maxAngularVelocity;

    var directionAV = {
        x: Math.random() - 0.5,
        y: Math.random() - 0.5,
        z: Math.random() - 0.5
    };
    // print("ROT magnitude is " + magnitudeAV + " and direction is " + directionAV.x);
    Entities.editEntity(entityId, {
        velocity: Vec3.multiply(magnitudeV, Vec3.normalize(directionV)),
        angularVelocity: Vec3.multiply(magnitudeAV, Vec3.normalize(directionAV))
    });

}

function update(deltaTime) {
    if (!initialized) {
        print("checking for servers...");
        if (Entities.serversExist() && Entities.canRez()) {
            print("servers exist -- makeAll...");
            Entities.setPacketsPerSecond(6000);
            print("PPS:" + Entities.getPacketsPerSecond());
            initialized = true;
            Script.setTimeout(findVesicles, 20 * 1000); // After 20 seconds of getting entities, look for cells.
        }
        return;
    }

    if (THROTTLE === true) {
        sinceLastUpdate = sinceLastUpdate + deltaTime * 1000;
        if (sinceLastUpdate > THROTTLE_RATE) {
            sinceLastUpdate = 0;
            entitiesToMove.forEach(function (v) {
                Script.setTimeout(function() {
                    moveVesicle(v);
                }, Math.random() * THROTTLE_RATE); // don't move all of them every five seconds, but at random times over interval
            });
        } else {
            return;
        }
    }

}

function unload() {
    Script.update.disconnect(update);
}

Script.update.connect(update);
Script.scriptEnding.connect(unload);
