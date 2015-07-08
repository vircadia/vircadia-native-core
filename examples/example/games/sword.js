//  stick.js
//  examples
//
//  Created by Seth Alves on 2015-6-10
//  Copyright 2015 High Fidelity, Inc.
//
//  Allow avatar to hold a stick
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
"use strict";
/*jslint vars: true*/
var Script, Entities, MyAvatar, Window, Overlays, Controller, Vec3, Quat, print; // Referenced globals provided by High Fidelity.

var hand = "right";
var nullActionID = "00000000-0000-0000-0000-000000000000";
var controllerID;
var controllerActive;
var stickID = null;
var actionID = nullActionID;
var dimensions = { x: 0.3, y: 0.1, z: 2.0 };
var AWAY_ORIENTATION =  Quat.fromPitchYawRollDegrees(-90, 0, 0);

var stickModel = "https://hifi-public.s3.amazonaws.com/eric/models/stick.fbx";
var swordModel = "https://hifi-public.s3.amazonaws.com/ozan/props/sword/sword.fbx";
var whichModel = "sword";
var rezButton = Overlays.addOverlay("image", {
    x: 100,
    y: 380,
    width: 32,
    height: 32,
    imageURL: "http://s3.amazonaws.com/hifi-public/images/delete.png",
    color: {
        red: 255,
        green: 255,
        blue: 255
    },
    alpha: 1
});

var health = 100;
var display;
var isAway = false;
function updateDisplay() {
    var text = health.toString();
    if (!display) {
        health = 100;
        display = Overlays.addOverlay("text", {
            text: text,
            font: { size: 20 },
            color: {red: 0, green: 255, blue: 0},
            backgroundColor: {red: 100, green: 100, blue: 100}, // Why doesn't this and the next work?
            backgroundAlpha: 0.9,
            x: Window.innerWidth - 50,
            y: 50
        });
    } else {
        Overlays.editOverlay(display, {text: text});
    }
}
function removeDisplay() {
    if (display) {
        Overlays.deleteOverlay(display);
        display = null;
    }
}

function cleanUp() {
    if (stickID) {
        Entities.deleteAction(stickID, actionID);
        Entities.deleteEntity(stickID);
        stickID = null;
        actionID = null;
    }
    removeDisplay();
    Overlays.deleteOverlay(rezButton);
}

function computeEnergy(collision, entityID) {
    var id = entityID || collision.idA || collision.idB;
    var entity = id && Entities.getEntityProperties(id);
    var mass = entity ? (entity.density * entity.dimensions.x * entity.dimensions.y * entity.dimensions.z) : 1;
    var linearVelocityChange = Vec3.length(collision.velocityChange);
    var energy = 0.5 * mass * linearVelocityChange * linearVelocityChange;
    return Math.min(Math.max(1.0, Math.round(energy)), 20);
}
function gotHit(collision) {
    if (isAway) { return; }
    var energy = computeEnergy(collision);
    health -= energy;
    updateDisplay();
}
function scoreHit(idA, idB, collision) {
    if (isAway) { return; }
    var energy = computeEnergy(collision, idA);
    health += energy;
    updateDisplay();
}

function positionStick(stickOrientation) {
    var baseOffset = {x: 0.3, y: 0.0, z: -dimensions.z / 2}; // FIXME: don't move yourself by colliding with your own capsule. Fudge of 0.3 in x.
    var offset = Vec3.multiplyQbyV(stickOrientation, baseOffset);
    Entities.updateAction(stickID, actionID, {relativePosition: offset,
                                              relativeRotation: stickOrientation});
}


function mouseMoveEvent(event) {
    if (!stickID || actionID === nullActionID || isAway) {
        return;
    }
    var windowCenterX = Window.innerWidth / 2;
    var windowCenterY = Window.innerHeight / 2;
    var mouseXCenterOffset = event.x - windowCenterX;
    var mouseYCenterOffset = event.y - windowCenterY;
    var mouseXRatio = mouseXCenterOffset / windowCenterX;
    var mouseYRatio = mouseYCenterOffset / windowCenterY;

    var stickOrientation = Quat.fromPitchYawRollDegrees(mouseYRatio * -90, mouseXRatio * -90, 0);
    positionStick(stickOrientation);
}


function initControls() {
    if (hand === "right") {
        controllerID = 3; // right handed
    } else {
        controllerID = 4; // left handed
    }
}


function update() {
    var palmPosition = Controller.getSpatialControlPosition(controllerID);
    controllerActive = (Vec3.length(palmPosition) > 0);
    if (!controllerActive) {
        return;
    }

    var stickOrientation = Controller.getSpatialControlRawRotation(controllerID);
    var adjustment = Quat.fromPitchYawRollDegrees(180, 0, 0);
    stickOrientation = Quat.multiply(stickOrientation, adjustment);

    positionStick(stickOrientation);
}

function toggleAway() {
    isAway = !isAway;
    if (isAway) {
        positionStick(AWAY_ORIENTATION);
        removeDisplay();
    } else {
        updateDisplay();
    }
}

function onClick(event) {
    switch (Overlays.getOverlayAtPoint({x: event.x, y: event.y})) {
    case rezButton:
        if (!stickID) {
            stickID = Entities.addEntity({
                type: "Model",
                modelURL: (whichModel === "sword") ? swordModel : stickModel,
                //compoundShapeURL: "https://hifi-public.s3.amazonaws.com/eric/models/stick.obj",
                shapeType: "box",
                dimensions: dimensions,
                position: MyAvatar.getRightPalmPosition(), // initial position doesn't matter, as long as it's close
                rotation: MyAvatar.orientation,
                damping: 0.1,
                collisionSoundURL: "http://public.highfidelity.io/sounds/Collisions-hitsandslaps/swordStrike1.wav",
                restitution: 0.01,
                collisionsWillMove: true
            });
            actionID = Entities.addAction("hold", stickID, {relativePosition: {x: 0.0, y: 0.0, z: -dimensions.z / 2},
                                                            hand: hand,
                                                            timeScale: 0.15});
            if (actionID === nullActionID) {
                print('*** FAILED TO MAKE SWORD ACTION ***');
                cleanUp();
            }
            Script.addEventHandler(stickID, 'collisionWithEntity', scoreHit);
            updateDisplay();
        } else {
            toggleAway();
        }
        break;
    }
}

Script.scriptEnding.connect(cleanUp);
Controller.mouseMoveEvent.connect(mouseMoveEvent);
Controller.mousePressEvent.connect(onClick);
Script.update.connect(update);
MyAvatar.collisionWithEntity.connect(gotHit);
