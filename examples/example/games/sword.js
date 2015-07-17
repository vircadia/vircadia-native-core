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
var Script, Entities, MyAvatar, Window, Overlays, Controller, Vec3, Quat, print, ToolBar, Settings; // Referenced globals provided by High Fidelity.
Script.include("http://s3.amazonaws.com/hifi-public/scripts/libraries/toolBars.js");

var hand = Settings.getValue("highfidelity.sword.hand", "right");
var nullActionID = "00000000-0000-0000-0000-000000000000";
var controllerID;
var controllerActive;
var stickID = null;
var actionID = nullActionID;
var targetIDs = [];
var dimensions = { x: 0.3, y: 0.15, z: 2.0 };
var BUTTON_SIZE = 32;

var stickModel = "https://hifi-public.s3.amazonaws.com/eric/models/stick.fbx";
var swordModel = "https://hifi-public.s3.amazonaws.com/ozan/props/sword/sword.fbx";
var swordCollisionShape = "https://hifi-public.s3.amazonaws.com/ozan/props/sword/sword.obj";
var swordCollisionSoundURL = "http://public.highfidelity.io/sounds/Collisions-hitsandslaps/swordStrike1.wav";
var avatarCollisionSoundURL = "https://s3.amazonaws.com/hifi-public/sounds/Collisions-hitsandslaps/airhockey_hit1.wav";
var whichModel = "sword";
var originalAvatarCollisionSound;

var toolBar = new ToolBar(0, 0, ToolBar.vertical, "highfidelity.sword.toolbar", function () {
    return {x: 100, y: 380};
});

var SWORD_IMAGE = "http://s3.amazonaws.com/hifi-public/images/billiardsReticle.png";  // Toggle between brandishing/sheathing sword (creating if necessary)
var TARGET_IMAGE = "http://s3.amazonaws.com/hifi-public/images/puck.png"; // Create a target dummy
var CLEANUP_IMAGE = "http://s3.amazonaws.com/hifi-public/images/delete.png"; // Remove sword and all target dummies.f
var SWITCH_HANDS_IMAGE = "http://s3.amazonaws.com/hifi-public/images/up-arrow.svg"; // Toggle left vs right hand. Persists in settings.
var swordButton = toolBar.addOverlay("image", {
    width: BUTTON_SIZE,
    height: BUTTON_SIZE,
    imageURL: SWORD_IMAGE,
    alpha: 1
});
var targetButton = toolBar.addOverlay("image", {
    width: BUTTON_SIZE,
    height: BUTTON_SIZE,
    imageURL: TARGET_IMAGE,
    alpha: 1
});
var switchHandsButton = toolBar.addOverlay("image", {
    width: BUTTON_SIZE,
    height: BUTTON_SIZE,
    imageURL: SWITCH_HANDS_IMAGE,
    alpha: 1
});
var cleanupButton = toolBar.addOverlay("image", {
    width: BUTTON_SIZE,
    height: BUTTON_SIZE,
    imageURL: CLEANUP_IMAGE,
    alpha: 1
});

var flasher;
function clearFlash() {
    if (!flasher) {
        return;
    }
    Script.clearTimeout(flasher.timer);
    Overlays.deleteOverlay(flasher.overlay);
    flasher = null;
}
function flash(color) {
    clearFlash();
    flasher = {};
    flasher.overlay = Overlays.addOverlay("text", {
        backgroundColor: color,
        backgroundAlpha: 0.7,
        width: Window.innerWidth,
        height: Window.innerHeight
    });
    flasher.timer = Script.setTimeout(clearFlash, 500);
}

var health = 100;
var display2d, display3d;
function trackAvatarWithText() {
    Entities.editEntity(display3d, {
        position: Vec3.sum(MyAvatar.position, {x: 0, y: 1.5, z: 0}),
        rotation: Quat.multiply(MyAvatar.orientation, Quat.fromPitchYawRollDegrees(0, 180, 0))
    });
}
function updateDisplay() {
    var text = health.toString();
    if (!display2d) {
        health = 100;
        display2d = Overlays.addOverlay("text", {
            text: text,
            font: { size: 20 },
            color: {red: 0, green: 255, blue: 0},
            backgroundColor: {red: 100, green: 100, blue: 100}, // Why doesn't this and the next work?
            backgroundAlpha: 0.9,
            x: toolBar.x - 5, // I'd like to add the score to the toolBar and have it drag with it, but toolBar doesn't support text (just buttons).
            y: toolBar.y - 30 // So next best thing is to position it each time as if it were on top.
        });
        display3d = Entities.addEntity({
            name: MyAvatar.displayName + " score",
            textColor: {red: 255, green: 255, blue: 255},
            type: "Text",
            text: text,
            lineHeight: 0.14,
            backgroundColor: {red: 64, green: 64, blue: 64},
            dimensions: {x: 0.3, y: 0.2, z: 0.01},
        });
        Script.update.connect(trackAvatarWithText);
    } else {
        Overlays.editOverlay(display2d, {text: text});
        Entities.editEntity(display3d, {text: text});
    }
}
function removeDisplay() {
    if (display2d) {
        Overlays.deleteOverlay(display2d);
        display2d = null;
        Script.update.disconnect(trackAvatarWithText);
        Entities.deleteEntity(display3d);
        display3d = null;
    }
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
    var energy = computeEnergy(collision);
    print("Got hit - " + energy + " from " + collision.idA + " " + collision.idB);
    health -= energy;
    flash({red: 255, green: 0, blue: 0});
    updateDisplay();
}
function scoreHit(idA, idB, collision) {
    var energy = computeEnergy(collision, idA);
    print("Score + " + energy + " from " + JSON.stringify(idA) + " " + JSON.stringify(idB));
    health += energy;
    flash({red: 0, green: 255, blue: 0});
    updateDisplay();
}

function isFighting() {
    return stickID && (actionID !== nullActionID);
}

function initControls() {
    print("Sword hand is " + hand);
    if (hand === "right") {
        controllerID = 3; // right handed
    } else {
        controllerID = 4; // left handed
    }
}
var inHand = false;
function positionStick(stickOrientation) {
    var reorient = Quat.fromPitchYawRollDegrees(0, -90, 0);
    var baseOffset = {x: -dimensions.z * 0.8, y: 0, z: 0};
    var offset = Vec3.multiplyQbyV(reorient, baseOffset);
    stickOrientation = Quat.multiply(reorient, stickOrientation);
    inHand = false;
    Entities.updateAction(stickID, actionID, {
        relativePosition: offset,
        relativeRotation: stickOrientation,
        hand: "right"
    });
}
function resetToHand() { // For use with controllers, puts the sword in contact with the hand.
    // Maybe coordinate with positionStick?
    if (inHand) { // Optimization: bail if we're already inHand.
        return;
    }
    print('Reset to hand');
    Entities.updateAction(stickID, actionID, {
        relativePosition: {x: 0.0, y: 0.0, z: -dimensions.z * 0.5},
        relativeRotation: Quat.fromVec3Degrees({x: 45.0, y: 0.0, z: 0.0}),
        hand: hand,   // It should not be necessary to repeat these two, but there seems to be a bug in that that
        timeScale: 0.05  // they do not retain their earlier values if you don't repeat them.
    });
    inHand = true;
}
function isControllerActive() {
    // I don't think the hydra API provides any reliable way to know whether a particular controller is active. Ask for both.
    controllerActive = (Vec3.length(Controller.getSpatialControlPosition(3)) > 0) || Vec3.length(Controller.getSpatialControlPosition(4)) > 0;
    return controllerActive;
}
function mouseMoveEvent(event) {
    // When a controller like the hydra gives a mouse event, the x/y is not meaningful to us, but we can detect with a truty deviceID
    if (event.deviceID || !isFighting() || isControllerActive()) {
        print('Attempting attachment reset');
        resetToHand();
        return;
    }
    var windowCenterX = Window.innerWidth / 2;
    var windowCenterY = Window.innerHeight / 2;
    var mouseXCenterOffset = event.x - windowCenterX;
    var mouseYCenterOffset = event.y - windowCenterY;
    var mouseXRatio = mouseXCenterOffset / windowCenterX;
    var mouseYRatio = mouseYCenterOffset / windowCenterY;

    var stickOrientation = Quat.fromPitchYawRollDegrees(mouseYRatio * 90, mouseXRatio * 90, 0);
    positionStick(stickOrientation);
}

function removeSword() {
    if (stickID) {
        print('deleting action ' + actionID + ' and entity ' + stickID);
        Entities.deleteAction(stickID, actionID);
        Entities.deleteEntity(stickID);
        stickID = null;
        actionID = nullActionID;
        Controller.mouseMoveEvent.disconnect(mouseMoveEvent);
        MyAvatar.collisionWithEntity.disconnect(gotHit);
        // removeEventhHandler happens automatically when the entity is deleted.
    }
    inHand = false;
    if (originalAvatarCollisionSound !== undefined) {
        MyAvatar.collisionSoundURL = originalAvatarCollisionSound;
    }
    removeDisplay();
}
function cleanUp(leaveButtons) {
    removeSword();
    targetIDs.forEach(function (id) {
        Entities.deleteAction(id.entity, id.action);
        Entities.deleteEntity(id.entity);
    });
    targetIDs = [];
    if (!leaveButtons) {
        toolBar.cleanup();
    }
}
function makeSword() {
    initControls();
    var swordPosition;
    if (!isControllerActive()) { // Dont' knock yourself with sword
        swordPosition = Vec3.sum(MyAvatar.position, Vec3.multiply(2, Quat.getFront(MyAvatar.orientation)));
    } else if (hand === 'right') {
        swordPosition = MyAvatar.getRightPalmPosition();
    } else {
        swordPosition = MyAvatar.getLeftPalmPosition();
    }
    stickID = Entities.addEntity({
        type: "Model",
        modelURL: swordModel,
        compoundShapeURL: swordCollisionShape,
        dimensions: dimensions,
        position: swordPosition,
        rotation: MyAvatar.orientation,
        damping: 0.1,
        collisionSoundURL: swordCollisionSoundURL,
        restitution: 0.01,
        collisionsWillMove: true
    });
    actionID = Entities.addAction("hold", stickID, {
        relativePosition: {x: 0.0, y: 0.0, z: -dimensions.z * 0.5},
        relativeRotation: Quat.fromVec3Degrees({x: 45.0, y: 0.0, z: 0.0}),
        hand: hand,
        timeScale: 0.05
    });
    if (actionID === nullActionID) {
        print('*** FAILED TO MAKE SWORD ACTION ***');
        cleanUp();
    }
    if (originalAvatarCollisionSound === undefined) {
        originalAvatarCollisionSound = MyAvatar.collisionSoundURL; // We won't get MyAvatar.collisionWithEntity unless there's a sound URL. (Bug.)
        SoundCache.getSound(avatarCollisionSoundURL); // Interface does not currently "preload" this? (Bug?)
    }
    MyAvatar.collisionSoundURL = avatarCollisionSoundURL;
    Controller.mouseMoveEvent.connect(mouseMoveEvent);
    MyAvatar.collisionWithEntity.connect(gotHit);
    Script.addEventHandler(stickID, 'collisionWithEntity', scoreHit);
    updateDisplay();
}

function onClick(event) {
    switch (Overlays.getOverlayAtPoint(event)) {
    case swordButton:
        if (!stickID) {
            makeSword();
        } else {
            removeSword();
        }
        break;
    case targetButton:
        var position = Vec3.sum(MyAvatar.position, {x: 1.0, y: 0.4, z: 0.0});
        var boxId = Entities.addEntity({
            type: "Box",
            name: "dummy",
            position: position,
            dimensions: {x: 0.3, y: 0.7, z: 0.3},
            gravity: {x: 0.0, y: -3.0, z: 0.0},
            damping: 0.2,
            collisionsWillMove: true
        });

        var pointToOffsetFrom = Vec3.sum(position, {x: 0.0, y: 2.0, z: 0.0});
        var action = Entities.addAction("offset", boxId, {pointToOffsetFrom: pointToOffsetFrom,
                                             linearDistance: 2.0,
                                             // linearTimeScale: 0.005
                                             linearTimeScale: 0.1
                                            });
        targetIDs.push({entity: boxId, action: action});
        break;
    case switchHandsButton:
        cleanUp('leaveButtons');
        hand = hand === "right" ? "left" : "right";
        Settings.setValue("highfidelity.sword.hand", hand);
        makeSword();
        break;
    case cleanupButton:
        cleanUp('leaveButtons');
        break;
    }
}

Script.scriptEnding.connect(cleanUp);
Controller.mousePressEvent.connect(onClick);
