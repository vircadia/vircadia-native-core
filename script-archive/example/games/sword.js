//  sword.js
//  examples
//
//  Created by Seth Alves on 2015-6-10
//  Copyright 2015 High Fidelity, Inc.
//
//  Allow avatar to hold a sword
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
"use strict";
/*jslint vars: true*/
var Script, Entities, MyAvatar, Window, Overlays, Controller, Vec3, Quat, print, ToolBar, Settings; // Referenced globals provided by High Fidelity.
Script.include("http://s3.amazonaws.com/hifi-public/scripts/libraries/toolBars.js");
var zombieGameScriptURL = "https://hifi-public.s3.amazonaws.com/eric/scripts/zombieFight.js?v2";
// var zombieGameScriptURL = "zombieFight.js";
Script.include(zombieGameScriptURL);


var zombieFight = new ZombieFight();

var hand = "right";
var zombieFight;
var nullActionID = "00000000-0000-0000-0000-000000000000";
var controllerID;
var controllerActive;
var swordID = null;
var actionID = nullActionID;
var dimensions = {
    x: 0.3,
    y: 0.15,
    z: 2.0
};
var BUTTON_SIZE = 32;

var health = 100;
var healthLossOnHit = 10;

var swordModel = "https://hifi-public.s3.amazonaws.com/ozan/props/sword/sword.fbx";
// var swordCollisionShape = "https://hifi-public.s3.amazonaws.com/ozan/props/sword/sword.obj";
var swordCollisionShape = "https://hifi-public.s3.amazonaws.com/eric/models/noHandleSwordCollisionShape.obj?=v2";
var swordCollisionSoundURL = "http://public.highfidelity.io/sounds/Collisions-hitsandslaps/swordStrike1.wav";
var avatarCollisionSoundURL = "https://hifi-public.s3.amazonaws.com/eric/sounds/blankSound.wav"; //Just to avoid no collision callback bug
var zombieGameScriptURL = "https://hifi-public.s3.amazonaws.com/eric/scripts/zombieFight.js";
var whichModel = "sword";
var originalAvatarCollisionSound;

var avatarCollisionSounds = [SoundCache.getSound("https://hifi-public.s3.amazonaws.com/eric/sounds/avatarHit.wav"), SoundCache.getSound("https://hifi-public.s3.amazonaws.com/eric/sounds/avatarHit2.wav?=v2")];

var toolBar = new ToolBar(0, 0, ToolBar.vertical, "highfidelity.sword.toolbar", function() {
    return {
        x: 100,
        y: 380
    };
});

var gameStarted = false;

var SWORD_IMAGE = "https://hifi-public.s3.amazonaws.com/images/sword/sword.svg"; // Toggle between brandishing/sheathing sword (creating if necessary)
var TARGET_IMAGE = "https://hifi-public.s3.amazonaws.com/images/sword/dummy2.svg"; // Create a target dummy
var CLEANUP_IMAGE = "http://s3.amazonaws.com/hifi-public/images/delete.png"; // Remove sword and all target dummies.f
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

var cleanupButton = toolBar.addOverlay("image", {
    width: BUTTON_SIZE,
    height: BUTTON_SIZE,
    imageURL: CLEANUP_IMAGE,
    alpha: 1
});

var flasher;

var leftHandClick = 14;
var leftTriggerValue = 0;
var prevLeftTriggerValue = 0;


var LEFT = 0;
var RIGHT = 1;

var leftPalm = 2 * LEFT;
var rightPalm = 2 * RIGHT;
var rightHandClick = 15;
var prevRightTriggerValue = 0;
var rightTriggerValue = 0;
var TRIGGER_THRESHOLD = 0.2;

var swordHeld = false;

function clearFlash() {
    if (!flasher) {
        return;
    }
    Script.clearTimeout(flasher.timer);
    Overlays.deleteOverlay(flasher.overlay);
    flasher = null;
}

function flash(color) {
    return;
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


var display2d, display3d;

function trackAvatarWithText() {
    Entities.editEntity(display3d, {
        position: Vec3.sum(MyAvatar.position, {
            x: 0,
            y: 1.5,
            z: 0
        }),
        rotation: Quat.multiply(MyAvatar.orientation, Quat.fromPitchYawRollDegrees(0, 180, 0))
    });
}

function updateDisplay() {
    var text = health.toString();
    if (!display2d) {
        display2d = Overlays.addOverlay("text", {
            text: text,
            font: {
                size: 20
            },
            color: {
                red: 0,
                green: 255,
                blue: 0
            },
            backgroundColor: {
                red: 100,
                green: 100,
                blue: 100
            }, // Why doesn't this and the next work?
            backgroundAlpha: 0.9,
            x: toolBar.x - 5, // I'd like to add the score to the toolBar and have it drag with it, but toolBar doesn't support text (just buttons).
            y: toolBar.y - 30 // So next best thing is to position it each time as if it were on top.
        });
        display3d = Entities.addEntity({
            name: MyAvatar.displayName + " score",
            textColor: {
                red: 255,
                green: 255,
                blue: 255
            },
            type: "Text",
            text: text,
            lineHeight: 0.14,
            backgroundColor: {
                red: 64,
                green: 64,
                blue: 64
            },
            dimensions: {
                x: 0.3,
                y: 0.2,
                z: 0.01
            },
        });
        Script.update.connect(trackAvatarWithText);
    } else {
        Overlays.editOverlay(display2d, {
            text: text
        });
        Entities.editEntity(display3d, {
            text: text
        });
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


function gotHit(collision) {
    Audio.playSound(avatarCollisionSounds[randInt(0, avatarCollisionSounds.length)], {
        position: MyAvatar.position,
        volume: 0.5
    });
    health -= healthLossOnHit;
    if (health <= 30) {
        Overlays.editOverlay(display2d, {
            color: {
                red: 200,
                green: 10,
                blue: 10
            }
        });
    }

    if (health <= 0 && zombieFight) {
        zombieFight.loseGame();
    }
    flash({
        red: 255,
        green: 0,
        blue: 0
    });
    updateDisplay();
}


function isFighting() {
    return swordID && (actionID !== nullActionID);
}


var inHand = false;


function isControllerActive() {
    // I don't think the hydra API provides any reliable way to know whether a particular controller is active. Ask for both.
    controllerActive = (Vec3.length(MyAvatar.leftHandPose.translation) > 0) || Vec3.length(MyAvatar.rightHandPose.translation) > 0;
    return controllerActive;
}


function removeSword() {
    if (swordID) {
        print('deleting action ' + actionID + ' and entity ' + swordID);
        Entities.deleteAction(swordID, actionID);
        Entities.deleteEntity(swordID);
        swordID = null;
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
    swordHeld = false;
}

function cleanUp(leaveButtons) {
    if (!leaveButtons) {
        toolBar.cleanup();
    }
    removeSword();
    gameStarted = false;
    zombieFight.cleanup();
}

function makeSword() {
    var swordPosition = Vec3.sum(MyAvatar.position, Vec3.multiply(5, Quat.getFront(MyAvatar.orientation)));
    var orientationAdjustment = Quat.fromPitchYawRollDegrees(90, 0, 0);

    swordID = Entities.addEntity({
        type: "Model",
        name: "sword",
        modelURL: swordModel,
        compoundShapeURL: swordCollisionShape,
        dimensions: dimensions,
        position: swordPosition,
        rotation: Quat.fromPitchYawRollDegrees(90, 0, 0),
        damping: 0.1,
        collisionSoundURL: swordCollisionSoundURL,
        restitution: 0.01,
        dynamic: true,
    });

    if (originalAvatarCollisionSound === undefined) {
        originalAvatarCollisionSound = MyAvatar.collisionSoundURL; // We won't get MyAvatar.collisionWithEntity unless there's a sound URL. (Bug.)
        SoundCache.getSound(avatarCollisionSoundURL); // Interface does not currently "preload" this? (Bug?)
    }

    if (!isControllerActive()) {
        grabSword("right");
    }
    MyAvatar.collisionSoundURL = avatarCollisionSoundURL;
    Controller.mouseMoveEvent.connect(mouseMoveEvent);
    MyAvatar.collisionWithEntity.connect(gotHit);
    updateDisplay();
}



function grabSword(hand) {
    if (!swordID) {
        print("Create a sword by clicking on sword icon!")
        return;
    }
    var handRotation;
    if (hand === "right") {
        handRotation = MyAvatar.rightHandPose.rotation;

    } else if (hand === "left") {
        handRotation = MyAvatar.leftHandPose.rotation;
    }
    var swordRotation = Entities.getEntityProperties(swordID).rotation;
    var offsetRotation = Quat.multiply(Quat.inverse(handRotation), swordRotation);
    actionID = Entities.addAction("hold", swordID, {
        relativePosition: {
            x: 0.0,
            y: 0.0,
            z: -dimensions.z * 0.5
        },
        relativeRotation: offsetRotation,
        hand: hand,
        timeScale: 0.05
    });
    if (actionID === nullActionID) {
        print('*** FAILED TO MAKE SWORD ACTION ***');
        cleanUp();
    } else {
        swordHeld = true;
    }
}

function releaseSword() {
    Entities.deleteAction(swordID, actionID);
    actionID = nullActionID;
    Entities.editEntity(swordID, {
        velocity: {
            x: 0,
            y: 0,
            z: 0
        },
        angularVelocity: {
            x: 0,
            y: 0,
            z: 0
        }
    });
    swordHeld = false;
}

function update() {
    updateControllerState();

}

function updateControllerState() {
    rightTriggerValue = Controller.getValue(Controller.Standard.RT);
    leftTriggerValue =Controller.getValue(Controller.Standard.LT);

    if (rightTriggerValue > TRIGGER_THRESHOLD && !swordHeld) {
        grabSword("right")
    } else if (rightTriggerValue < TRIGGER_THRESHOLD && prevRightTriggerValue > TRIGGER_THRESHOLD && swordHeld) {
        releaseSword();
    }

    if (leftTriggerValue > TRIGGER_THRESHOLD && !swordHeld) {
        grabSword("left")
    } else if (leftTriggerValue < TRIGGER_THRESHOLD && prevLeftTriggerValue > TRIGGER_THRESHOLD && swordHeld) {
        releaseSword();
    }

    prevRightTriggerValue = rightTriggerValue;
    prevLeftTriggerValue = leftTriggerValue;
}

randFloat = function(low, high) {
    return low + Math.random() * (high - low);
}


randInt = function(low, high) {
    return Math.floor(randFloat(low, high));
}

function positionSword(swordOrientation) {
    var reorient = Quat.fromPitchYawRollDegrees(0, -90, 0);
    var baseOffset = {
        x: -dimensions.z * 0.8,
        y: 0,
        z: 0
    };
    var offset = Vec3.multiplyQbyV(reorient, baseOffset);
    swordOrientation = Quat.multiply(reorient, swordOrientation);
    inHand = false;
    Entities.updateAction(swordID, actionID, {
        relativePosition: offset,
        relativeRotation: swordOrientation,
        hand: "right"
    });
}

function resetToHand() { // For use with controllers, puts the sword in contact with the hand.
    // Maybe coordinate with positionSword?
    if (inHand) { // Optimization: bail if we're already inHand.
        return;
    }
    print('Reset to hand');
    Entities.updateAction(swordID, actionID, {
        relativePosition: {
            x: 0.0,
            y: 0.0,
            z: -dimensions.z * 0.5
        },
        relativeRotation: Quat.fromVec3Degrees({
            x: 45.0,
            y: 0.0,
            z: 0.0
        }),
        hand: "right", // It should not be necessary to repeat these two, but there seems to be a bug in that that
        timeScale: 0.05 // they do not retain their earlier values if you don't repeat them.
    });
    inHand = true;
}

function mouseMoveEvent(event) {
    //When a controller like the hydra gives a mouse event, the x/y is not meaningful to us, but we can detect with a truty deviceID
    if (event.deviceID || !isFighting() || isControllerActive()) {
        resetToHand();
        return;
    }
    var windowCenterX = Window.innerWidth / 2;
    var windowCenterY = Window.innerHeight / 2;
    var mouseXCenterOffset = event.x - windowCenterX;
    var mouseYCenterOffset = event.y - windowCenterY;
    var mouseXRatio = mouseXCenterOffset / windowCenterX;
    var mouseYRatio = mouseYCenterOffset / windowCenterY;

    var swordOrientation = Quat.fromPitchYawRollDegrees(mouseYRatio * 90, mouseXRatio * 90, 0);
    positionSword(swordOrientation);
}


function onClick(event) {
    switch (Overlays.getOverlayAtPoint(event)) {
        case swordButton:
            if (!swordID) {
                makeSword();
            } else {
                removeSword();
            }
            break;
        case targetButton:
            if (gameStarted) {
                return;
            }
            Script.include("https://hifi-public.s3.amazonaws.com/eric/scripts/zombieFight.js");
            zombieFight = new ZombieFight();
            zombieFight.initiateZombieApocalypse();
            gameStarted = true;

            break;
        case cleanupButton:
            cleanUp('leaveButtons');
            break;
    }
}

Script.scriptEnding.connect(cleanUp);
Script.update.connect(update);
Controller.mousePressEvent.connect(onClick);
