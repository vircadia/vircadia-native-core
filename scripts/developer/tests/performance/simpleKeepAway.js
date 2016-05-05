"use strict";
/*jslint nomen: true, plusplus: true, vars: true*/
var Vec3, Quat, MyAvatar, Entities, Camera, Script, print;
//
//  Created by Howard Stearns
//  Copyright 2016 High Fidelity, Inc.
//  
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
//  Drops a bunch of physical spheres which each run an entity script that moves away
//  from you once a second if you are within range.
//
//  This is a test of how many physical, entity-scripted objects can be around if
//  they are mostly not doing anything -- i.e., just the basic overhead of such objects.
//  They do need a moment to settle out of active physics after being dropped, but only
//  a moment -- that's why they are in a sparse grid.

var USE_FLAT_FLOOR = true;  // Give 'em a flat place to settle on.
var ROWS_X = 30;
var ROWS_Z = 30;
var SEPARATION = 1;         // meters
var LIFETIME = 60;         // seconds
var DISTANCE_RATE = 1;      // hz
var DISTANCE_ALLOWANCE = 3; // meters. Must be this close to cause entity to move
var DISTANCE_SCALE = 0.5;   // velocity will be scale x vector from user to entity

var SIZE = 0.5;
var TYPE = "Sphere";
//  Note that when creating things quickly, the entity server will ignore data if we send updates too quickly.
//  like Internet MTU, these rates are set by th domain operator, so in this script there is a RATE_PER_SECOND 
//  variable letting you set this speed.  If entities are missing from the grid after a relog, this number 
//  being too high may be the reason. 
var RATE_PER_SECOND = 600;    //    The entity server will drop data if we create things too fast.
var SCRIPT_INTERVAL = 100;

var GRAVITY = { x: 0, y: -9.8, z: 0 };
var VELOCITY = { x: 0.0, y: 0.1, z: 0 };

var DAMPING = 0.75;
var ANGULAR_DAMPING = 0.75;

var RANGE = 3;
var HOW_FAR_UP = 2;  // for uneven ground

var x = 0;
var z = 0;
var totalCreated = 0;
var xDim = ROWS_X * SEPARATION;
var zDim = ROWS_Z * SEPARATION;
var totalToCreate = ROWS_X * ROWS_Z;
var origin = Vec3.sum(MyAvatar.position, {x: xDim / -2, y: HOW_FAR_UP, z: zDim / -2});
print("Creating " + totalToCreate + " origined on " + JSON.stringify(MyAvatar.position) +
      ", starting at " + JSON.stringify(origin));
var parameters = JSON.stringify({
    distanceRate: DISTANCE_RATE,
    distanceScale: DISTANCE_SCALE,
    distanceAllowance: DISTANCE_ALLOWANCE
});

var startTime = new Date();
if (USE_FLAT_FLOOR) {
    Entities.addEntity({
        type: 'Box',
        name: 'keepAwayFloor',
        lifetime: LIFETIME,
        collisionsWillMove: false,
        color: {red: 255, green: 0, blue: 0},
        position: Vec3.sum(origin, {x: xDim / 2, y: -1 - HOW_FAR_UP, z: zDim / 2}),
        dimensions: {x: xDim + SEPARATION, y: 0.2, z: zDim + SEPARATION}
    });
}
Script.setInterval(function () {
    if (!Entities.serversExist() || !Entities.canRez()) {
        return;
    }

    var i, properties, numToCreate = Math.min(RATE_PER_SECOND * (SCRIPT_INTERVAL / 1000.0), totalToCreate - totalCreated);
    for (i = 0; i < numToCreate; i++) {
        properties = {
            userData: parameters,
            type: TYPE,
            name: "keepAway-" + totalCreated,
            position: {
                x: origin.x + SIZE + (x * SEPARATION),
                y: origin.y,
                z: origin.z + SIZE + (z * SEPARATION)
            },
            dimensions: {x: SIZE, y: SIZE, z: SIZE},
            color: {red: Math.random() * 255, green: Math.random() * 255, blue: Math.random() * 255},
            velocity: VELOCITY,
            damping: DAMPING,
            angularDamping: ANGULAR_DAMPING,
            gravity: GRAVITY,
            collisionsWillMove: true,
            lifetime: LIFETIME,
            script: Script.resolvePath("keepAwayEntity.js")
        };
        Entities.addEntity(properties);
        totalCreated++;

        x++;
        if (x === ROWS_X) {
            x = 0;
            z++;
            print("Created: " + totalCreated);
        }
        if (z === ROWS_Z) {
            print("Total: " + totalCreated + " entities in " + ((new Date() - startTime) / 1000.0) + " seconds.");
            Script.stop();
        }
    }
}, SCRIPT_INTERVAL);
