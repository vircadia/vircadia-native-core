//
//  gun.js
//  examples
//
//  Created by Brad Hefta-Gaub on 12/31/13.
//  Modified by Philip on 3/3/14
//  Modified by Thijs Wenker on 3/31/15
//  Copyright 2013 High Fidelity, Inc.
//
//  This is an example script that turns the hydra controllers and mouse into a entity gun.
//  It reads the controller, watches for trigger pulls, and launches entities.
//  When entities collide with voxels they blow little holes out of the voxels. 
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

// FIXME kickback functionality was removed because the joint setting interface in 
// MyAvatar has apparently changed, breaking it.  

Script.include("../libraries/utils.js");
Script.include("../libraries/constants.js");
Script.include("../libraries/toolBars.js");

HIFI_PUBLIC_BUCKET = "http://s3.amazonaws.com/hifi-public/";

var LASER_WIDTH = 2;
var POSE_CONTROLS = [ Controller.Standard.LeftHand, Controller.Standard.RightHand ];
var TRIGGER_CONTROLS = [ Controller.Standard.LT, Controller.Standard.RT ];
var MIN_THROWER_DELAY = 1000;
var MAX_THROWER_DELAY = 1000;
var RELOAD_INTERVAL = 5;
var GUN_MODEL = HIFI_PUBLIC_BUCKET + "cozza13/gun/m1911-handgun+1.fbx?v=4";
var BULLET_VELOCITY = 10.0;
var GUN_OFFSETS = [ {
    x: -0.04,
    y: 0.26,
    z: 0.04
}, {
    x: 0.04,
    y: 0.26,
    z: 0.04
} ];

var GUN_ORIENTATIONS = [ Quat.fromPitchYawRollDegrees(0, 90, 90), Quat.fromPitchYawRollDegrees(0, -90, 270) ];

var BARREL_OFFSETS = [ {
    x: -0.12,
    y: 0.12,
    z: 0.04
}, {
    x: 0.12,
    y: 0.12,
    z: 0.04
} ];

var mapping = Controller.newMapping();
var validPoses = [ false, false ];
var barrelVectors = [ 0, 0 ];
var barrelTips = [ 0, 0 ];
var pointer = [];

pointer.push(Overlays.addOverlay("line3d", {
    start: ZERO_VECTOR,
    end: ZERO_VECTOR,
    color: COLORS.RED,
    alpha: 1,
    visible: true,
    lineWidth: LASER_WIDTH
}));

pointer.push(Overlays.addOverlay("line3d", {
    start: ZERO_VECTOR,
    end: ZERO_VECTOR,
    color: COLORS.RED,
    alpha: 1,
    visible: true,
    lineWidth: LASER_WIDTH
}));

function getRandomFloat(min, max) {
    return Math.random() * (max - min) + min;
}

// Load some sound to use for loading and firing
var fireSound = SoundCache.getSound(HIFI_PUBLIC_BUCKET + "sounds/Guns/GUN-SHOT2.raw");
var loadSound = SoundCache.getSound(HIFI_PUBLIC_BUCKET + "sounds/Guns/Gun_Reload_Weapon22.raw");
var impactSound = SoundCache.getSound(HIFI_PUBLIC_BUCKET + "sounds/Guns/BulletImpact2.raw");
var targetHitSound = SoundCache.getSound(HIFI_PUBLIC_BUCKET + "sounds/Space%20Invaders/hit.raw");
var targetLaunchSound = SoundCache.getSound(HIFI_PUBLIC_BUCKET + "sounds/Space%20Invaders/shoot.raw");

var audioOptions = {
    volume: 0.9
}

var shotsFired = 0;
var shotTime = new Date();
var isLaunchButtonPressed = false;
var score = 0;

var bulletID = false;
var targetID = false;

// Create overlay buttons and reticle
var BUTTON_SIZE = 32;
var PADDING = 3;
var NUM_BUTTONS = 3;
var screenSize = Controller.getViewportDimensions();
var startX = screenSize.x / 2 - (NUM_BUTTONS * (BUTTON_SIZE + PADDING)) / 2;
var toolBar = new ToolBar(0, 0, ToolBar.HORIZONTAL, "highfidelity.gun.toolbar", function(screenSize) {
    return {
        x: startX,
        y: (screenSize.y - (BUTTON_SIZE + PADDING)),
    };
});



function shootBullet(position, velocity, grenade) {
    var BULLET_SIZE = .09;
    var BULLET_LIFETIME = 10.0;
    var BULLET_GRAVITY = -0.25;
    var GRENADE_VELOCITY = 15.0;
    var GRENADE_SIZE = 0.35;
    var GRENADE_GRAVITY = -9.8;

    var bVelocity = grenade ? Vec3.multiply(GRENADE_VELOCITY, Vec3.normalize(velocity)) : velocity;
    var bSize = grenade ? GRENADE_SIZE : BULLET_SIZE;
    var bGravity = grenade ? GRENADE_GRAVITY : BULLET_GRAVITY;

    bulletID = Entities.addEntity({
        type: "Sphere",
        position: position,
        dimensions: {
            x: bSize,
            y: bSize,
            z: bSize
        },
        color: {
            red: 0,
            green: 0,
            blue: 0
        },
        velocity: bVelocity,
        lifetime: BULLET_LIFETIME,
        gravity: {
            x: 0,
            y: bGravity,
            z: 0
        },
        damping: 0.01,
        density: 8000,
        ignoreCollisions: false,
        collisionsWillMove: true
    });

    Script.addEventHandler(bulletID, "collisionWithEntity", entityCollisionWithEntity);

    // Play firing sounds
    audioOptions.position = position;
    Audio.playSound(fireSound, audioOptions);
    shotsFired++;
    if ((shotsFired % RELOAD_INTERVAL) == 0) {
        Audio.playSound(loadSound, audioOptions);
    }
}

function keyPressEvent(event) {
    // if our tools are off, then don't do anything
    if (event.text == "t") {
        var time = MIN_THROWER_DELAY + Math.random() * MAX_THROWER_DELAY;
        Script.setTimeout(shootTarget, time);
    } else if ((event.text == ".") || (event.text == "SPACE")) {
        shootFromMouse(false);
    } else if (event.text == ",") {
        shootFromMouse(true);
    } else if (event.text == "r") {
        playLoadSound();
    }
}

function playLoadSound() {
    audioOptions.position = MyAvatar.leftHandPose.translation;
    Audio.playSound(loadSound, audioOptions);
}

function update(deltaTime) {
    // FIXME we should also expose MyAvatar.handPoses[2], MyAvatar.tipPoses[2]
    var tipPoses = [ MyAvatar.leftHandTipPose, MyAvatar.rightHandTipPose ];

    for (var side = 0; side < 2; side++) {
        // First check if the controller is valid
        var controllerPose = Controller.getPoseValue(POSE_CONTROLS[side]);
        validPoses[side] = controllerPose.valid;
        if (!controllerPose.valid) {
            Overlays.editOverlay(pointer[side], {
                visible: false
            });
            continue;
        }

        // Need to adjust the laser
        var tipPose = tipPoses[side];
        var handRotation = tipPoses[side].rotation;
        var barrelOffset = Vec3.multiplyQbyV(handRotation, BARREL_OFFSETS[side]);
        barrelTips[side] = Vec3.sum(tipPose.translation, barrelOffset);
        barrelVectors[side] = Vec3.multiplyQbyV(handRotation, {
            x: 0,
            y: 1,
            z: 0
        });

        var laserTip = Vec3.sum(Vec3.multiply(100.0, barrelVectors[side]), barrelTips[side]);
        // Update Lasers
        Overlays.editOverlay(pointer[side], {
            start: barrelTips[side],
            end: laserTip,
            alpha: 1,
            visible: true
        });
    }
}

function triggerChanged(side, value) {
    var pressed = (value != 0);
    if (pressed) {
        var position = barrelTips[side];
        var velocity = Vec3.multiply(BULLET_VELOCITY, Vec3.normalize(barrelVectors[side]));
        shootBullet(position, velocity, false);
    }
}

function mousePressEvent(event) {
    var clickedOverlay = Overlays.getOverlayAtPoint({
        x: event.x,
        y: event.y
    });
    if (clickedOverlay == offButton) {
        Script.stop();
    } else if (clickedOverlay == platformButton) {
        var platformSize = 3;
        makePlatform(-9.8, 1.0, platformSize);
    } else if (clickedOverlay == gridButton) {
        makeGrid("Box", 1.0, 3);
    }
}

function scriptEnding() {
    mapping.disable();
    toolBar.cleanup();
    for (var i = 0; i < pointer.length; ++i) {
        Overlays.deleteOverlay(pointer[i]);
    }
    MyAvatar.detachOne(GUN_MODEL);
    MyAvatar.detachOne(GUN_MODEL);
    clearPose();
}

MyAvatar.attach(GUN_MODEL, "LeftHand", GUN_OFFSETS[0], GUN_ORIENTATIONS[0], 0.40);
MyAvatar.attach(GUN_MODEL, "RightHand", GUN_OFFSETS[1], GUN_ORIENTATIONS[1], 0.40);

// Give a bit of time to load before playing sound
Script.setTimeout(playLoadSound, 2000);

mapping.from(Controller.Standard.LT).hysteresis(0.1, 0.5).to(function(value) {
    triggerChanged(0, value);
});

mapping.from(Controller.Standard.RT).hysteresis(0.1, 0.5).to(function(value) {
    triggerChanged(1, value);
});
mapping.enable();

Script.scriptEnding.connect(scriptEnding);
Script.update.connect(update);
Controller.mousePressEvent.connect(mousePressEvent);
Controller.keyPressEvent.connect(keyPressEvent);
