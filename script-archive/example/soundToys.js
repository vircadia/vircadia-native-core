"use strict";
//  Creates some objects that each play a sound when they are hit (or when they hit something else).
//
//  Created by Howard Stearns on June 3, 2015
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html

var Camera, Vec3, Quat, Entities, Script; // Globals defined by HiFi, var'ed here to keep jslint happy.
var 
var VIRCADIA_PUBLIC_CDN = networkingConstants.PUBLIC_BUCKET_CDN_URL;
var SOUND_BUCKET = "http://public.highfidelity.io/sounds/Collisions-hitsandslaps/";
var MAX_ANGULAR_SPEED = Math.PI;
var N_EACH_OBJECTS = 3;

var ourToys = [];
function deleteAll() {
    ourToys.forEach(Entities.deleteEntity);
}
function makeAll() {
    var currentPosition = Vec3.sum(Camera.getPosition(), Vec3.multiply(4, Quat.getFront(Camera.getOrientation()))),
        right = Vec3.multiply(0.6, Quat.getRight(Camera.getOrientation())),
        currentDimensions,
        data = [
            ["models/props/Dice/goldDie.fbx", VIRCADIA_PUBLIC_CDN + "sounds/dice/diceCollide.wav"],
            ["models/props/Pool/ball_8.fbx", VIRCADIA_PUBLIC_CDN + "sounds/Collisions-ballhitsandcatches/billiards/collision1.wav"],
            ["eric/models/woodFloor.fbx", SOUND_BUCKET + "67LCollision05.wav"]
        ];
    currentPosition = Vec3.sum(currentPosition, Vec3.multiply(-1 * data.length * N_EACH_OBJECTS / 2, right));
    function makeOne(model, sound) {
        var thisEntity;
        function dropOnce() { // Once gravity is added, it will work if picked up and again dropped.
            Entities.editEntity(thisEntity, {gravity: {x: 0, y: -9.8, z: 0}});
            Script.removeEventHandler(thisEntity, 'clickDownOnEntity', dropOnce);
        }
        thisEntity = Entities.addEntity({
            type: "Model",
            modelURL: VIRCADIA_PUBLIC_CDN + model,
            collisionSoundURL: sound,
            dynamic: true,
            shapeType: "box",
            restitution: 0.8,
            dimensions: currentDimensions,
            position: currentPosition,
            angularVelocity: {
                x: Math.random() * MAX_ANGULAR_SPEED,
                y: Math.random() * MAX_ANGULAR_SPEED,
                z: Math.random() * MAX_ANGULAR_SPEED
            }
        });
        ourToys.push(thisEntity);
        Script.addEventHandler(thisEntity, 'clickDownOnEntity', dropOnce);
        currentDimensions = Vec3.multiply(currentDimensions, 2);
        currentPosition = Vec3.sum(currentPosition, right);
    }
    function makeThree(modelSound) {
        var i, model = modelSound[0], sound = modelSound[1];
        currentDimensions = {x: 0.1, y: 0.1, z: 0.1};
        for (i = 0; i < N_EACH_OBJECTS; i++) {
            makeOne(model, sound);
        }
    }
    data.forEach(makeThree);
}
makeAll();
Script.scriptEnding.connect(deleteAll);
