"use strict";
/* jslint vars: true, plusplus: true, forin: true*/
/* globals Script, Entities, MyAvatar, Vec3, Quat */
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

var LIFETIME = 3600;
var BALL_SIZE = 0.175;
var BALL_DAMPING = 0.5;
var BALL_ANGULAR_DAMPING = 0.5;
var BALL_RESTITUTION = 0.4;
var BALL_DENSITY = 1000;
var ACTION_DISTANCE = 0.35;
var ACTION_TIMESCALE = 0.035;
var MAX_DISTANCE_MULTIPLIER = 4;
var STICK_SCRIPT_URL = Script.resolvePath("./entity_scripts/tetherballStick.js");
var STICK_MODEL_URL = Script.getExternalPath(Script.ExternalPaths.HF_Content, "/caitlyn/production/raveStick/newRaveStick2.fbx");
var COLLISION_SOUND_URL = "http://public.highfidelity.io/sounds/Footsteps/FootstepW3Left-12db.wav";

var avatarOrientation = MyAvatar.orientation;
avatarOrientation = Quat.safeEulerAngles(avatarOrientation);
avatarOrientation.x = 0;
avatarOrientation = Quat.fromVec3Degrees(avatarOrientation);
var front = Quat.getFront(avatarOrientation);
var stickStartPosition = Vec3.sum(MyAvatar.getRightPalmPosition(), front);
var ballStartPosition = Vec3.sum(stickStartPosition, Vec3.multiply(0.36, front));

var ballID = Entities.addEntity({
    type: "Model",
    modelURL: Script.getExternalPath(Script.ExternalPaths.HF_Content, "/Examples%20Content/production/marblecollection/Star.fbx"),
    name: "TetherballStick Ball",
    shapeType: "Sphere",
    position: ballStartPosition,
    lifetime: LIFETIME,
    collisionSoundURL: COLLISION_SOUND_URL,
    dimensions: {
        x: BALL_SIZE,
        y: BALL_SIZE,
        z: BALL_SIZE
    },
    gravity: {
        x: 0.0,
        y: -9.8,
        z: 0.0
    },
    damping: BALL_DAMPING,
    angularDamping: BALL_ANGULAR_DAMPING,
    density: BALL_DENSITY,
    restitution: BALL_RESTITUTION,
    dynamic: true,
    collidesWith: "static,dynamic,otherAvatar,",
    grab: { grabbable: false }
});

var lineID = Entities.addEntity({
    type: "PolyLine",
    name: "TetherballStick Line",
    color: {
        red: 0,
        green: 120,
        blue: 250
    },
    textures: Script.getExternalPath(Script.ExternalPaths.HF_Public, "/alan/Particles/Particle-Sprite-Smoke-1.png"),
    position: ballStartPosition,
    dimensions: {
        x: 10,
        y: 10,
        z: 10
    },
    lifetime: LIFETIME
});

var actionID = Entities.addAction("offset", ballID, {
    pointToOffsetFrom: stickStartPosition,
    linearDistance: ACTION_DISTANCE,
    linearTimeScale: ACTION_TIMESCALE
});

var STICK_PROPERTIES = {
    type: 'Model',
    name: "TetherballStick Stick",
    modelURL: STICK_MODEL_URL,
    position: stickStartPosition,
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
    lifetime: LIFETIME,
    grab: {
        grabbable: false,
        grabFollowsController: false,
        equippable: true,
        equippableLeftPosition: {
            x: -0.14998853206634521,
            y: 0.17033983767032623,
            z: 0.023199155926704407
        },
        equippableLeftRotation: {
            x: 0.6623835563659668,
            y: -0.1671387255191803,
            z: 0.7071226835250854,
            w: 0.1823924481868744
        },
        equippableRightPosition: {
            x: 0.15539926290512085,
            y: 0.14493153989315033,
            z: 0.023641478270292282
        },
        equippableRightRotation: {
            x: 0.5481458902359009,
            y: -0.4470711946487427,
            z: -0.3148134648799896,
            w: 0.6328644752502441
        }
    },
    ownerID: MyAvatar.sessionUUID,
    ballID: ballID,
    lineID: lineID,
    actionID: actionID,
    maxDistanceBetweenBallAndStick: ACTION_DISTANCE * MAX_DISTANCE_MULTIPLIER
};

Entities.addEntity(STICK_PROPERTIES);
Script.stop();
