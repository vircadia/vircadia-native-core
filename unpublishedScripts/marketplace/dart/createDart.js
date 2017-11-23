"use strict";
//
// createDart.js
//
// Created by MrRoboman on 17/05/04
// Copyright 2017 High Fidelity, Inc.
//
// Creates five throwing darts.
//
// Distributed under the Apache License, Version 2.0.
// See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html

var MODEL_URL = "https://hifi-content.s3.amazonaws.com/wadewatts/dart.fbx";
var SCRIPT_URL = Script.resolvePath("./dart.js?v=" + Date.now());
var START_POSITION = Vec3.sum(MyAvatar.position, Vec3.multiply(Quat.getFront(MyAvatar.orientation), 0.75));
var START_ROTATION = MyAvatar.orientation
var SCALE_FACTOR = 1;

var dart = {
    type: "Model",
    shapeType: "box",
    name: "Dart",
    description: "Throw it at something!",
    script: SCRIPT_URL,
    modelURL: MODEL_URL,
    position: START_POSITION,
    rotation: START_ROTATION,
    lifetime: 300,
    gravity: {
        x: 0,
        y: -9.8,
        z: 0
    },
    dimensions: {
        x: 0.1,
        y: 0.1,
        z: 0.3
    },
    dynamic: true,
    owningAvatarID: MyAvatar.sessionUUID,
    userData: JSON.stringify({
        grabbableKey: {
            grabbable: true,
            invertSolidWhileHeld: true,
            ignoreIK: false
        }
    })
};

var avatarUp = Quat.getUp(MyAvatar.orientation);
var platformPosition = Vec3.sum(START_POSITION, Vec3.multiply(avatarUp, -0.05));
var platform = {
    type: "Box",
    name: "Dart Platform",
    description: "Holds darts",
    position: platformPosition,
    rotation: START_ROTATION,
    lifetime: 60,
    dimensions: {
        x: 0.15 * 5,
        y: 0.01,
        z: 0.3
    },
    color: {
        red: 129,
        green: 92,
        blue: 11
    },
    owningAvatarID: MyAvatar.sessionUUID,
    userData: JSON.stringify({
        grabbableKey: {
            grabbable: true,
            invertSolidWhileHeld: true,
            ignoreIK: false
        }
    })
};


Entities.addEntity(platform);

var dartCount = 5;
var avatarRight = Quat.getRight(MyAvatar.orientation);
for (var i = 0; i < dartCount; i++) {
    var j = i - Math.floor(dartCount / 2);
    var position = Vec3.sum(START_POSITION, Vec3.multiply(avatarRight, 0.15 * j));
    dart.position = position;
    Entities.addEntity(dart);
}

Script.stop();
