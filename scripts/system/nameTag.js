"use strict";

/*jslint vars: true, plusplus: true*/
/*global Entities, Script, Quat, Vec3, MyAvatar, print*/
// nameTag.js
//
// Created by Triplelexx on 17/01/31
// Copyright 2017 High Fidelity, Inc.
//
// Running the script creates a text entity that will hover over the user's head showing their display name.
//
// Distributed under the Apache License, Version 2.0.
// See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html

const CLIENTONLY = false;
const NULL_UUID = "{00000000-0000-0000-0000-000000000000}";
const ENTITY_CHECK_INTERVAL = 5000; // ms = 5 seconds
const STARTUP_DELAY = 2000; // ms = 2 second
const OLD_AGE = 3500; // we recreate the entity if older than this time in seconds 
const TTL = 2; // time to live in seconds if script is not running
const HEIGHT_ABOVE_HEAD = 0.2;
const HEAD_OFFSET = -0.025;
const SIZE_Y = 0.075;
const LETTER_OFFSET = 0.03; // arbitrary value to dynamically change width, could be more accurate by detecting characters
const LINE_HEIGHT = 0.05;

var nameTagEntityID = NULL_UUID;
var lastCheckForEntity = 0;

// create the name tag entity after a brief delay
Script.setTimeout(function() {
    addNameTag();
}, STARTUP_DELAY);

function addNameTag() {
    var nameTagPosition = Vec3.sum(MyAvatar.getHeadPosition(), Vec3.multiply(HEAD_OFFSET, Quat.getForward(MyAvatar.orientation)));
    nameTagPosition.y += HEIGHT_ABOVE_HEAD;
    var nameTagProperties = {
        name: MyAvatar.displayName + ' Name Tag',
        type: 'Text',
        text: MyAvatar.displayName,
        lineHeight: LINE_HEIGHT,
        parentID: MyAvatar.sessionUUID,
        dimensions: dimensionsFromName(),
        position: nameTagPosition
    }
    nameTagEntityID = Entities.addEntity(nameTagProperties, CLIENTONLY);
}

function updateNameTag() {
    var nameTagProps = Entities.getEntityProperties(nameTagEntityID);
    var nameTagPosition = Vec3.sum(MyAvatar.getHeadPosition(), Vec3.multiply(HEAD_OFFSET, Quat.getForward(MyAvatar.orientation)));
    nameTagPosition.y += HEIGHT_ABOVE_HEAD;

    Entities.editEntity(nameTagEntityID, {
        position: nameTagPosition,
        dimensions: dimensionsFromName(),
        // lifetime is in seconds we add TTL on top of the next poll time
        lifetime: Math.round(nameTagProps.age) + (ENTITY_CHECK_INTERVAL / 1000) + TTL,
        text: MyAvatar.displayName
    });
};

function deleteNameTag() {
    if(nameTagEntityID !== NULL_UUID) {
        Entities.deleteEntity(nameTagEntityID);
        nameTagEntityID = NULL_UUID;
    }
}

function dimensionsFromName() {
    return {
        x: LETTER_OFFSET * MyAvatar.displayName.length,
        y: SIZE_Y,
        z: 0.0
    }
};

// cleanup on ending
Script.scriptEnding.connect(cleanup);
function cleanup() {
    deleteNameTag();
}

Script.update.connect(update);
function update() {
    // if no entity we return
    if(nameTagEntityID == NULL_UUID) {
        return;
    }

    if(Date.now() - lastCheckForEntity > ENTITY_CHECK_INTERVAL) {
        checkForEntity();
        lastCheckForEntity = Date.now();
    }
}

function checkForEntity() {
    var nameTagProps = Entities.getEntityProperties(nameTagEntityID);
    // it is possible for the age to not be a valid number, we check for this and return accordingly
    if(nameTagProps.age == -1) {
        return;
    }

    // it's too old or we receive undefined make a new one, otherwise update
    if(nameTagProps.age > OLD_AGE || nameTagProps.age == undefined) {
        deleteNameTag();
        addNameTag();
    } else {
        updateNameTag();
    }
}
