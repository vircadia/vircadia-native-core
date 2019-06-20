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
//  Drops a bunch of physical spheres in front of you, each running a script that will:
//  * Edit color at EDIT_RATE for EDIT_TIMEOUT.
//  * Randomly move at an average of MOVE_RATE for MOVE_TIMEOUT.
//  The _TIMEOUT parameters can be 0 for no activity, and -1 to be active indefinitely.
//

var NUMBER_TO_CREATE = 100;
var LIFETIME = 120; // seconds
var EDIT_RATE = 60; // hz
var EDIT_TIMEOUT = -1;
var MOVE_RATE = 1; // hz
var MOVE_TIMEOUT = LIFETIME / 2;

var SIZE = 0.5;
var TYPE = "Sphere";
//  Note that when creating things quickly, the entity server will ignore data if we send updates too quickly.
//  like Internet MTU, these rates are set by th domain operator, so in this script there is a RATE_PER_SECOND 
//  variable letting you set this speed.  If entities are missing from the grid after a relog, this number 
//  being too high may be the reason. 
var RATE_PER_SECOND = 600;    //    The entity server will drop data if we create things too fast.
var SCRIPT_INTERVAL = 100;

var GRAVITY = { x: 0, y: -9.8, z: 0 };
var VELOCITY = { x: 0.0, y: 0, z: 0 };
var ANGULAR_VELOCITY = { x: 1, y: 1, z: 1 };

var DAMPING = 0.5;
var ANGULAR_DAMPING = 0.5;

var RANGE = 3;
var HOW_FAR_IN_FRONT_OF_ME = RANGE * 3;
var HOW_FAR_UP = RANGE / 1.5;  // higher (for uneven ground) above range/2 (for distribution)

var totalCreated = 0;
var offset = Vec3.sum(Vec3.multiply(HOW_FAR_UP, Vec3.UNIT_Y),
                      Vec3.multiply(HOW_FAR_IN_FRONT_OF_ME, Quat.getForward(Camera.orientation)));
var center = Vec3.sum(MyAvatar.position, offset);

function randomVector(range) {
    return {
        x: (Math.random() -  0.5) * range.x,
        y: (Math.random() -  0.5) * range.y,
        z: (Math.random() -  0.5) * range.z
    };
}

if (!Entities.canRezTmp()) {
    Window.alert("Cannot create temp objects here.");
    Script.stop();
} else {
    Script.setInterval(function () {
        if (!Entities.serversExist()) {
            return;
        }
        if (totalCreated >= NUMBER_TO_CREATE) {
            print("Created " + totalCreated + " tribbles.");
            Script.stop();
        }

        var i, numToCreate = RATE_PER_SECOND * (SCRIPT_INTERVAL / 1000.0);
        var parameters = JSON.stringify({
            moveTimeout: MOVE_TIMEOUT,
            moveRate: MOVE_RATE,
            editTimeout: EDIT_TIMEOUT,
            editRate: EDIT_RATE,
            debug: {flow: false, send: false, receive: false}
        });
        for (i = 0; (i < numToCreate) && (totalCreated < NUMBER_TO_CREATE); i++) {
            Entities.addEntity({
                userData: parameters,
                type: TYPE,
                name: "tribble-" + totalCreated,
                position: Vec3.sum(center, randomVector({ x: RANGE, y: RANGE, z: RANGE })),
                dimensions: {x: SIZE, y: SIZE, z: SIZE},
                color: {red: Math.random() * 255, green: Math.random() * 255, blue: Math.random() * 255},
                velocity: VELOCITY,
                angularVelocity: Vec3.multiply(Math.random(), ANGULAR_VELOCITY),
                damping: DAMPING,
                angularDamping: ANGULAR_DAMPING,
                gravity: GRAVITY,
                collisionsWillMove: true,
                lifetime: LIFETIME,
                script: Script.resolvePath("tribbleEntity.js")
            });

            totalCreated++;
        }
    }, SCRIPT_INTERVAL);
}
