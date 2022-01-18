//
//  createSingleBasketball.js
//  examples
//
//  Created by Philip Rosedale on August 20, 2015
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//


var basketballURL ="https://cdn-1.vircadia.com/us-e-1/DomainContent/Toybox/basketball/basketball2.fbx";
var collisionSoundURL ="https://cdn-1.vircadia.com/us-e-1/DomainContent/Toybox/basketball/basketball.wav";


var basketball = null;
var originalPosition = null;
var hasMoved = false;

var GRAVITY = -9.8;
var DISTANCE_IN_FRONT_OF_ME = 1.0;
var START_MOVE = 0.01;
var DIAMETER = 0.30;

function makeBasketball() {
    var position = Vec3.sum(MyAvatar.position,
        Vec3.multiplyQbyV(MyAvatar.orientation, {
            x: 0,
            y: 0.0,
            z: -DISTANCE_IN_FRONT_OF_ME
        }));
    var rotation = Quat.multiply(MyAvatar.orientation,
        Quat.fromPitchYawRollDegrees(0, -90, 0));
    basketball = Entities.addEntity({
        type: "Model",
        position: position,
        rotation: rotation,
        dimensions: {
            x: DIAMETER,
            y: DIAMETER,
            z: DIAMETER
        },
        dynamic: true,
        collisionSoundURL: collisionSoundURL,
        modelURL: basketballURL,
        restitution: 1.0,
        damping: 0.00001,
        shapeType: "sphere",
        userData: JSON.stringify({
            grabbableKey: {
                invertSolidWhileHeld: true
            }
        })
    });
    originalPosition = position;
}

function update() {
    if (!basketball) {
        makeBasketball();
    } else {
        var newProperties = Entities.getEntityProperties(basketball);
        var moved = Vec3.length(Vec3.subtract(originalPosition, newProperties.position));
        if (!hasMoved && (moved > START_MOVE)) {
            hasMoved = true;
            Entities.editEntity(basketball, {
                gravity: {
                    x: 0,
                    y: GRAVITY,
                    z: 0
                }
            });
        }
        var MAX_DISTANCE = 10.0;
        var distance = Vec3.length(Vec3.subtract(MyAvatar.position, newProperties.position));
        if (distance > MAX_DISTANCE) {
            deleteStuff();
        }
    }
}

function scriptEnding() {
    deleteStuff();
}

function deleteStuff() {
    if (basketball != null) {
        Entities.deleteEntity(basketball);
        basketball = null;
        hasMoved = false;
    }
}

Script.update.connect(update);
Script.scriptEnding.connect(scriptEnding);
