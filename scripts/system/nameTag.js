"use strict";

/*jslint vars: true, plusplus: true*/
/*global Entities, Script, Quat, Vec3, MyAvatar, print*/
// nameTag.js
//
// Created by Triplelexx on 17/01/31
// Copyright 2017 High Fidelity, Inc.
//
//
// Distributed under the Apache License, Version 2.0.
// See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html

const NAMETAG_DIMENSIONS = {
    x: 1.0,
    y: 0.2,
    z: 1.0
}

const CLIENTONLY = false;
const NULL_UUID = "{00000000-0000-0000-0000-000000000000}";
const ENTITY_CHECK_INTERVAL = 5000; // ms = 5 seconds
const STARTUP_DELAY = 2000; // ms = 2 second
const OLD_AGE = 3500; // we recreate the entity if older than this time in seconds 
const TTL = 2; // time to live in seconds if script is not running
const HEIGHT_ABOVE_HEAD = 0.2;
const HEAD_OFFSET = -0.025;

var nametagEntityID = NULL_UUID;
var lastCheckForEntity = 0;

// create the name tag entity after a brief delay
Script.setTimeout(function() {
    addNameTag();
}, STARTUP_DELAY);

function addNameTag() {
    var nametagPosition = Vec3.sum(MyAvatar.getHeadPosition(), Vec3.multiply(HEAD_OFFSET, Quat.getFront(MyAvatar.orientation)));
    nametagPosition.y += HEIGHT_ABOVE_HEAD;
    var modelNameTagProperties = {
        type: 'Text',
        text: MyAvatar.displayName,
        parentID: MyAvatar.sessionUUID,
        dimensions: NAMETAG_DIMENSIONS,
        position: nametagPosition
    }
    nametagEntityID = Entities.addEntity(modelNameTagProperties, CLIENTONLY);
}

function deleteNameTag() {
    if(nametagEntityID !== NULL_UUID) {
        Entities.deleteEntity(nametagEntityID);
        nametagEntityID = NULL_UUID;
    }
}

// cleanup on ending
Script.scriptEnding.connect(cleanup);
function cleanup() {
    deleteNameTag();
}

Script.update.connect(update);
function update() {
    // bail if no entity
    if(nametagEntityID == NULL_UUID) {
        return;
    }

    if(Date.now() - lastCheckForEntity > ENTITY_CHECK_INTERVAL) {
        checkForEntity();
        lastCheckForEntity = Date.now();
    }
}

function checkForEntity() {
    var nametagProps = Entities.getEntityProperties(nametagEntityID);

    // it is possible for the age to not be a valid number, we check for this and bail accordingly
    if(nametagProps.age < 1) {
        return;
    }
    
    // it's too old make a new one, otherwise update
    if(nametagProps.age > OLD_AGE || nametagProps.age == undefined) {
        deleteNameTag();
        addNameTag();
    } else {
        var nametagPosition = Vec3.sum(MyAvatar.getHeadPosition(), Vec3.multiply(HEAD_OFFSET, Quat.getFront(MyAvatar.orientation)));
        nametagPosition.y += HEIGHT_ABOVE_HEAD;
        Entities.editEntity(nametagEntityID, {
            position: nametagPosition,
            // lifetime is in seconds we add TTL on top of the next poll time
            lifetime: Math.round(nametagProps.age) + (ENTITY_CHECK_INTERVAL / 1000) + TTL,
            text: MyAvatar.displayName
        });
    }
}
