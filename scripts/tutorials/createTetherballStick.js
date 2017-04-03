"use strict";
/* jslint vars: true, plusplus: true, forin: true*/
/* globals Tablet, Script, AvatarList, Users, Entities, MyAvatar, Camera, Overlays, Vec3, Quat, Controller, print, getControllerWorldLocation */
/* eslint indent: ["error", 4, { "outerIIFEBody": 0 }] */
//
// createTetherballStick.js
//
// Created by Triplelexx on 17/03/04
// Updated by MrRoboman on 17/03/26
// Copyright 2017 High Fidelity, Inc.
//
// Creates an equippable stick with a tethered ball
//
// Distributed under the Apache License, Version 2.0.
// See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html

var STICK_SCRIPT_URL = Script.resolvePath("./entity_scripts/tetherballStick.js?v=" + Date.now());
var STICK_MODEL_URL = "http://hifi-content.s3.amazonaws.com/caitlyn/production/raveStick/newRaveStick2.fbx";

var avatarOrientation = MyAvatar.orientation;
avatarOrientation = Quat.safeEulerAngles(avatarOrientation);
avatarOrientation.x = 0;
avatarOrientation = Quat.fromVec3Degrees(avatarOrientation);
var startPosition = Vec3.sum(MyAvatar.getRightPalmPosition(), Vec3.multiply(1, Quat.getFront(avatarOrientation)));

var STICK_PROPERTIES = {
    type: 'Model',
    name: "TetherballStick Stick",
    modelURL: STICK_MODEL_URL,
    position: startPosition,
    rotation: MyAvatar.orientation,
    dimensions: {
        x: 0.0651,
        y: 0.0651,
        z: 0.5270
    },
    script: STICK_SCRIPT_URL,
    color: {
        red: 200,
        green: 0,
        blue: 20
    },
    shapeType: 'box',
    lifetime: 3600,
    userData: JSON.stringify({
        grabbableKey: {
            grabbable: true,
            spatialKey: {
                rightRelativePosition: {
                    x: 0.05,
                    y: 0,
                    z: 0
                },
                leftRelativePosition: {
                    x: -0.05,
                    y: 0,
                    z: 0
                },
                relativeRotation: {
                    x: 0.4999999701976776,
                    y: 0.4999999701976776,
                    z: -0.4999999701976776,
                    w: 0.4999999701976776
                }
            },
            invertSolidWhileHeld: true
        },
        ownerID: MyAvatar.sessionUUID
    })
};

Entities.addEntity(STICK_PROPERTIES);
Script.stop();
