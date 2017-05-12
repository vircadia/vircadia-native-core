"use strict";
/* jslint vars: true, plusplus: true, forin: true*/
/* globals Tablet, Script, AvatarList, Users, Entities, MyAvatar, Camera, Overlays, Vec3, Quat, Controller, print, getControllerWorldLocation */
/* eslint indent: ["error", 4, { "outerIIFEBody": 0 }] */
//
// createFloatinLanternBox.js
//
// Created by MrRoboman on 17/05/04
// Copyright 2017 High Fidelity, Inc.
//
// Creates a crate that spawn floating lanterns
//
// Distributed under the Apache License, Version 2.0.
// See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html

var COMPOUND_SHAPE_URL = "http://hifi-content.s3.amazonaws.com/Examples%20Content/production/maracas/woodenCrate_phys.obj";
var MODEL_URL = "http://hifi-content.s3.amazonaws.com/Examples%20Content/production/maracas/woodenCrate_VR.fbx";
var SCRIPT_URL = Script.resolvePath("./entity_scripts/floatingLanternBox.js?v=" + Date.now());
var START_POSITION = Vec3.sum(MyAvatar.position, Vec3.multiply(Quat.getFront(MyAvatar.orientation), 2));
START_POSITION.y -= .6;
var LIFETIME = 3600;
var SCALE_FACTOR = 1;

var lanternBox = {
    type: "Model",
    name: "Floating Lantern Box",
    description: "Spawns Lanterns that float away when grabbed and released!",
    script: SCRIPT_URL,
    modelURL: MODEL_URL,
    shapeType: "Compound",
    compoundShapeURL: COMPOUND_SHAPE_URL,
    position: START_POSITION,
    lifetime: LIFETIME,
    dimensions: {
        x: 0.8696 * SCALE_FACTOR,
        y: 0.58531 * SCALE_FACTOR,
        z: 0.9264 * SCALE_FACTOR
    },
    owningAvatarID: MyAvatar.sessionUUID
};

Entities.addEntity(lanternBox);
Script.stop();
