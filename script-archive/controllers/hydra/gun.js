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

Script.include("../../libraries/utils.js");
Script.include("../../libraries/constants.js");
Script.include("../../libraries/toolBars.js");


var VIRCADIA_PUBLIC_CDN = networkingConstants.PUBLIC_BUCKET_CDN_URL;

var LASER_WIDTH = 2;
var POSE_CONTROLS = [ Controller.Standard.LeftHand, Controller.Standard.RightHand ];
var TRIGGER_CONTROLS = [ Controller.Standard.LT, Controller.Standard.RT ];
var MIN_THROWER_DELAY = 1000;
var MAX_THROWER_DELAY = 1000;
var RELOAD_INTERVAL = 5;
var GUN_MODEL = VIRCADIA_PUBLIC_CDN + "cozza13/gun/m1911-handgun+1.fbx?v=4";
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

var showScore = false;
// Load some sound to use for loading and firing
var fireSound = SoundCache.getSound(VIRCADIA_PUBLIC_CDN + "sounds/Guns/GUN-SHOT2.raw");
var loadSound = SoundCache.getSound(VIRCADIA_PUBLIC_CDN + "sounds/Guns/Gun_Reload_Weapon22.raw");
var impactSound = SoundCache.getSound(VIRCADIA_PUBLIC_CDN + "sounds/Guns/BulletImpact2.raw");
var targetHitSound = SoundCache.getSound(VIRCADIA_PUBLIC_CDN + "sounds/Space%20Invaders/hit.raw");
var targetLaunchSound = SoundCache.getSound(VIRCADIA_PUBLIC_CDN + "sounds/Space%20Invaders/shoot.raw");

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

var offButton = toolBar.addOverlay("image", {
    width: BUTTON_SIZE,
    height: BUTTON_SIZE,
    imageURL: VIRCADIA_PUBLIC_CDN + "images/gun/close.svg",
    alpha: 1
});

startX += BUTTON_SIZE + PADDING;
var platformButton = toolBar.addOverlay("image", {
    x: startX,
    y: screenSize.y - (BUTTON_SIZE + PADDING),
    width: BUTTON_SIZE,
    height: BUTTON_SIZE,
    imageURL: VIRCADIA_PUBLIC_CDN + "images/gun/platform-targets.svg",
    alpha: 1
});

startX += BUTTON_SIZE + PADDING;
var gridButton = toolBar.addOverlay("image", {
    x: startX,
    y: screenSize.y - (BUTTON_SIZE + PADDING),
    width: BUTTON_SIZE,
    height: BUTTON_SIZE,
    imageURL: VIRCADIA_PUBLIC_CDN + "images/gun/floating-targets.svg",
    alpha: 1
});

if (showScore) {
    var text = Overlays.addOverlay("text", {
        x: screenSize.x / 2 - 100,
        y: screenSize.y / 2 - 50,
        width: 150,
        height: 50,
        color: {
            red: 0,
            green: 0,
            blue: 0
        },
        textColor: {
            red: 255,
            green: 0,
            blue: 0
        },
        topMargin: 4,
        leftMargin: 4,
        text: "Score: " + score
    });
}

function entityCollisionWithEntity(entity1, entity2, collision) {
    if (entity2 === targetID) {
        score++;
        if (showScore) {
            Overlays.editOverlay(text, {
                text: "Score: " + score
            });
        }

        // We will delete the bullet and target in 1/2 sec, but for now we can
        // see them bounce!
        Script.setTimeout(deleteBulletAndTarget, 500);

        // Turn the target and the bullet white
        Entities.editEntity(entity1, {
            color: {
                red: 255,
                green: 255,
                blue: 255
            }
        });
        Entities.editEntity(entity2, {
            color: {
                red: 255,
                green: 255,
                blue: 255
            }
        });

        // play the sound near the camera so the shooter can hear it
        audioOptions.position = Vec3.sum(Camera.getPosition(), Quat.getFront(Camera.getOrientation()));
        Audio.playSound(targetHitSound, audioOptions);
    }
}

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
        dynamic: true
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

function shootTarget() {
    var TARGET_SIZE = 0.50;
    var TARGET_GRAVITY = 0.0;
    var TARGET_LIFETIME = 300.0;
    var TARGET_UP_VELOCITY = 0.0;
    var TARGET_FWD_VELOCITY = 0.0;
    var DISTANCE_TO_LAUNCH_FROM = 5.0;
    var ANGLE_RANGE_FOR_LAUNCH = 20.0;
    var camera = Camera.getPosition();

    var targetDirection = Quat.angleAxis(getRandomFloat(-ANGLE_RANGE_FOR_LAUNCH, ANGLE_RANGE_FOR_LAUNCH), {
        x: 0,
        y: 1,
        z: 0
    });
    targetDirection = Quat.multiply(Camera.getOrientation(), targetDirection);
    var forwardVector = Quat.getFront(targetDirection);

    var newPosition = Vec3.sum(camera, Vec3.multiply(forwardVector, DISTANCE_TO_LAUNCH_FROM));

    var velocity = Vec3.multiply(forwardVector, TARGET_FWD_VELOCITY);
    velocity.y += TARGET_UP_VELOCITY;

    targetID = Entities.addEntity({
        type: "Box",
        position: newPosition,
        dimensions: {
            x: TARGET_SIZE * (0.5 + Math.random()),
            y: TARGET_SIZE * (0.5 + Math.random()),
            z: TARGET_SIZE * (0.5 + Math.random()) / 4.0
        },
        color: {
            red: Math.random() * 255,
            green: Math.random() * 255,
            blue: Math.random() * 255
        },
        velocity: velocity,
        gravity: {
            x: 0,
            y: TARGET_GRAVITY,
            z: 0
        },
        lifetime: TARGET_LIFETIME,
        rotation: Camera.getOrientation(),
        damping: 0.1,
        density: 100.0,
        dynamic: true
    });

    // Record start time
    shotTime = new Date();

    // Play target shoot sound
    audioOptions.position = newPosition;
    Audio.playSound(targetLaunchSound, audioOptions);
}

function makeGrid(type, scale, size) {
    var separation = scale * 2;
    var pos = Vec3.sum(Camera.getPosition(), Vec3.multiply(10.0 * scale * separation, Quat.getFront(Camera.getOrientation())));
    var x, y, z;
    var GRID_LIFE = 60.0;
    var dimensions;

    for (x = 0; x < size; x++) {
        for (y = 0; y < size; y++) {
            for (z = 0; z < size; z++) {

                dimensions = {
                    x: separation / 2.0 * (0.5 + Math.random()),
                    y: separation / 2.0 * (0.5 + Math.random()),
                    z: separation / 2.0 * (0.5 + Math.random()) / 4.0
                };

                Entities.addEntity({
                    type: type,
                    position: {
                        x: pos.x + x * separation,
                        y: pos.y + y * separation,
                        z: pos.z + z * separation
                    },
                    dimensions: dimensions,
                    color: {
                        red: Math.random() * 255,
                        green: Math.random() * 255,
                        blue: Math.random() * 255
                    },
                    velocity: {
                        x: 0,
                        y: 0,
                        z: 0
                    },
                    gravity: {
                        x: 0,
                        y: 0,
                        z: 0
                    },
                    lifetime: GRID_LIFE,
                    rotation: Camera.getOrientation(),
                    damping: 0.1,
                    density: 100.0,
                    dynamic: true
                });
            }
        }
    }
}

function makePlatform(gravity, scale, size) {
    var separation = scale * 2;
    var pos = Vec3.sum(Camera.getPosition(), Vec3.multiply(10.0 * scale * separation, Quat.getFront(Camera.getOrientation())));
    pos.y -= separation * size;
    var x, y, z;
    var TARGET_LIFE = 60.0;
    var INITIAL_GAP = 0.5;
    var dimensions;

    for (x = 0; x < size; x++) {
        for (y = 0; y < size; y++) {
            for (z = 0; z < size; z++) {

                dimensions = {
                    x: separation / 2.0,
                    y: separation,
                    z: separation / 2.0
                };

                Entities.addEntity({
                    type: "Box",
                    position: {
                        x: pos.x - (separation * size / 2.0) + x * separation,
                        y: pos.y + y * (separation + INITIAL_GAP),
                        z: pos.z - (separation * size / 2.0) + z * separation
                    },
                    dimensions: dimensions,
                    color: {
                        red: Math.random() * 255,
                        green: Math.random() * 255,
                        blue: Math.random() * 255
                    },
                    velocity: {
                        x: 0,
                        y: 0.05,
                        z: 0
                    },
                    gravity: {
                        x: 0,
                        y: gravity,
                        z: 0
                    },
                    lifetime: TARGET_LIFE,
                    damping: 0.1,
                    density: 100.0,
                    dynamic: true
                });
            }
        }
    }

    // Make a floor for this stuff to fall onto
    Entities.addEntity({
        type: "Box",
        position: {
            x: pos.x,
            y: pos.y - separation / 2.0,
            z: pos.z
        },
        dimensions: {
            x: 2.0 * separation * size,
            y: separation / 2.0,
            z: 2.0 * separation * size
        },
        color: {
            red: 100,
            green: 100,
            blue: 100
        },
        lifetime: TARGET_LIFE
    });

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

function deleteBulletAndTarget() {
    Entities.deleteEntity(bulletID);
    Entities.deleteEntity(targetID);
    bulletID = false;
    targetID = false;
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
    Overlays.deleteOverlay(text);
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
