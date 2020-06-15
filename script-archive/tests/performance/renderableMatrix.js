"use strict";
/*jslint nomen: true, plusplus: true, vars: true*/
var Entities, Script, print, Vec3, MyAvatar, Camera, Quat;
//
//  Created by Howard Stearns
//  Copyright 2016 High Fidelity, Inc.
//  
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
//  Creates a rectangular matrix of objects with no physical or entity changes after creation.
//  Useful for testing the rendering, LOD, and octree storage aspects of the system.
//
//  NOTE: to test the full rendering of the specified number of objects (as opposed to
//  testing LOD filtering), you may want to set LOD to manual maximum visibility.

var LIFETIME = 120;
var ROWS_X = 17;
var ROWS_Y = 10;
var ROWS_Z = 10;
// Entities will be populated from this list set by the script writer for different tests.
var TYPES_TO_USE = ['Box', 'Sphere'];
switch ('primitives') { // Quickly override the above by putting here one of the following case strings.
    case 'particles':
        TYPES_TO_USE = ['ParticleEffect'];
        ROWS_X = ROWS_Z = 6;
        break;
    case 'lights':
        TYPES_TO_USE = ['Light'];
        ROWS_X = ROWS_Y = ROWS_Z = 5;
        break;
    case 'blocks': // 376 triangles/entity
        TYPES_TO_USE = ["http://s3.amazonaws.com/hifi-public/marketplace/hificontent/Games/blocks/block.fbx"];
        ROWS_X = ROWS_Y = ROWS_Z = 10;
        break;
    case 'ramp': // 1,002 triangles/entity
        TYPES_TO_USE = ["http://headache.hungry.com/~seth/hifi/curved-ramp.obj"];
        ROWS_X = ROWS_Y = 10;
        ROWS_Z = 9;
        break;
    case 'gun': // 2.1k triangles/entity
        TYPES_TO_USE = ["https://hifi-content.s3.amazonaws.com/ozan/dev/props/guns/nail_gun/nail_gun.fbx"];
        ROWS_X = ROWS_Y = 10;
        ROWS_Z = 7;
        break;
    case 'trees': // 12k triangles/entity
        TYPES_TO_USE = ["https://hifi-content.s3.amazonaws.com/ozan/dev/sets/lowpoly_island/CypressTreeGroup.fbx"];
        ROWS_X = ROWS_Z = 6;
        ROWS_Y = 1;
        break;
    case 'web':
        TYPES_TO_USE = ['Web'];
        ROWS_X = ROWS_Z = 5;
        ROWS_Y = 3;
        break;
    default:
        // Just using values from above.
}
// Matrix will be axis-aligned, approximately all in this field of view.
// As special case, if zero, grid is centered above your head.
var MINIMUM_VIEW_ANGLE_IN_RADIANS = 30 * Math.PI / 180; // 30 degrees
var SEPARATION = 10;
var SIZE = 1;
var MODEL_SCALE = { x: 1, y: 2, z: 3 }; // how to stretch out models proportionally to SIZE
//  Note that when creating things quickly, the entity server will ignore data if we send updates too quickly.
//  like Internet MTU, these rates are set by th domain operator, so in this script there is a RATE_PER_SECOND 
//  variable letting you set this speed.  If entities are missing from the grid after a relog, this number 
//  being too high may be the reason. 
var RATE_PER_SECOND = 1000;    // The entity server will drop data if we create things too fast.
var SCRIPT_INTERVAL = 100;

var ALLOWED_TYPES = ['Box', 'Sphere', 'Light', 'ParticleEffect', 'Web']; // otherwise assumed to be a model url

var x = 0;
var y = 0;
var z = 0;
var xDim = ROWS_X * SEPARATION;
var yDim = ROWS_Y * SEPARATION;
var zDim = ROWS_Z * SEPARATION;
var centered = !MINIMUM_VIEW_ANGLE_IN_RADIANS;
var approximateNearDistance = !centered &&
    Math.max(xDim, 2 * yDim, zDim) / (2 * Math.tan(MINIMUM_VIEW_ANGLE_IN_RADIANS / 2)); // matrix is up, not vertically centered
var o = Vec3.sum(MyAvatar.position,
                 Vec3.sum({x: xDim / -2, y: 0, z: zDim / -2},
                          centered ? {x: 0, y: SEPARATION, z: 0} : Vec3.multiply(approximateNearDistance, Quat.getFront(Camera.orientation))));
var totalCreated = 0;
var startTime = new Date();
var totalToCreate = ROWS_X * ROWS_Y * ROWS_Z;
print("Creating " + totalToCreate + " " + JSON.stringify(TYPES_TO_USE) +
      " entities extending in positive x/y/z from " + JSON.stringify(o) +
      ", starting at " + startTime);

Script.setInterval(function () {
    if (!Entities.serversExist() || !Entities.canRez()) {
        return;
    }

    var numToCreate = Math.min(RATE_PER_SECOND * (SCRIPT_INTERVAL / 1000.0), totalToCreate - totalCreated);
    var chooseTypeRandomly = TYPES_TO_USE.length !== 2;
    var i, typeIndex, type, isModel, properties;
    for (i = 0; i < numToCreate; i++) {
        typeIndex = chooseTypeRandomly ? Math.floor(Math.random() * TYPES_TO_USE.length) : i % TYPES_TO_USE.length;
        type = TYPES_TO_USE[typeIndex];
        isModel = ALLOWED_TYPES.indexOf(type) === -1;
        properties = {
            position: { x: o.x + SIZE + (x * SEPARATION), y: o.y + SIZE + (y * SEPARATION), z: o.z + SIZE + (z * SEPARATION) },
            name: "renderable-" + x + "-" + y + "-" + z,
            type: isModel ? 'Model' : type,
            dimensions: isModel ? Vec3.multiply(SIZE, MODEL_SCALE) : { x: SIZE, y: SIZE, z: SIZE },
            ignoreCollisions: true,
            collisionsWillMove: false,
            lifetime: LIFETIME
        };
        if (isModel) {
            properties.modelURL = type;
        } else if (type === 'Web') {
            properties.sourceUrl = 'https://vircadia.com';
        } else {
            properties.color = { red: x / ROWS_X * 255, green: y / ROWS_Y * 255, blue: z / ROWS_Z * 255 };
            if (type === 'ParticleEffect') {
                properties.emitOrientation = Quat.fromPitchYawRollDegrees(-90.0, 0.0, 0.0);
                properties.particleRadius = 0.04;
                properties.radiusSpread = 0.0;
                properties.emitRate = 100;
                properties.emitSpeed = 1;
                properties.speedSpread = 0.0;
                properties.emitAcceleration = { x: 0.0, y: -0.3, z: 0.0 };
                properties.accelerationSpread = { x: 0.0, y: 0.0, z: 0.0 };
                properties.textures = "https://hifi-public.s3.amazonaws.com/alan/Particles/Particle-Sprite-Smoke-1.png";
                properties.lifespan = 5.0;
                properties.colorStart = properties.color;
                properties.colorFinish = properties.color;
                properties.alphaFinish = 0.0;
                properties.polarFinish = 2.0 * Math.PI / 180;
            } else if (type === 'Light') {
                properties.dimensions = Vec3.multiply(SEPARATION, properties.position);
            }
        }
        Entities.addEntity(properties);
        totalCreated++;

        x++;
        if (x === ROWS_X) {
            x = 0;
            y++;
            if (y === ROWS_Y) {
                y = 0;
                z++;
                print("Created: " + totalCreated);
            }
        }
        if (z === ROWS_Z) {
            print("Total: " + totalCreated + " entities in " + ((new Date() - startTime) / 1000.0) + " seconds.");
            Script.stop();
        }
    }
}, SCRIPT_INTERVAL);
