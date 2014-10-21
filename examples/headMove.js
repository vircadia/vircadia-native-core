//
//  headMove.js
//  examples
//
//  Created by Philip Rosedale on September 8, 2014
//  Copyright 2014 High Fidelity, Inc.
//
//  Press the spacebar and then use your head to move and turn.  Pull back to see your body. 
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

var debug = false;
var willMove = false;

var warpActive = false;
var warpPosition = { x: 0, y: 0, z: 0 };

//  Overlays to show target location 

var WARP_SPHERE_SIZE = 0.15;
var warpSphere = Overlays.addOverlay("sphere", {
    position: { x: 0, y: 0, z: 0 },
    size: WARP_SPHERE_SIZE,
    color: { red: 200, green: 0, blue: 0 },
    alpha: 0.5,
    solid: true,
    visible: false,
});

var WARP_LINE_HEIGHT = 5;
var warpLine = Overlays.addOverlay("line3d", {
    position: { x: 0, y: 0, z:0 },
    end: { x: 0, y: 0, z: 0 },
    color: { red: 0, green: 255, blue: 255},
    alpha: 1,
    lineWidth: 5,
    visible: false,
});

var movingWithHead = false; 
var headStartPosition, headStartDeltaPitch, headStartFinalPitch, headStartRoll, headStartYaw;
var timeSinceLastTurn = 0.0;
var timeSinceLastStep = 0.0;
var isTurning = false; 
var deltaYaw = 0.0;
var turnAmount = 0.0;
var keyDownTime = 0.0;
var watchAvatar = false; 
var oldMode; 

function saveCameraState() {
    oldMode = Camera.getMode();
    Camera.setMode("independent");
}

function restoreCameraState() {
    //Camera.stopLooking();
    Camera.setMode(oldMode);
}

function activateWarp() {
    if (warpActive) return;
    warpActive = true;

    updateWarp();
}

var WARP_PITCH_DEAD_ZONE = 4.0;
var WARP_SENSITIVITY = 0.35;
var TRIGGER_PULLBACK_DISTANCE = 0.04;
var WATCH_AVATAR_DISTANCE = 1.5;
var MAX_WARP_YAW = 40.0;
var MAX_PULLBACK_YAW = 5.0;

var sound = new Sound("http://public.highfidelity.io/sounds/Footsteps/FootstepW2Right-12db.wav");
function playSound() {
    var options = new AudioInjectionOptions();
    var position = MyAvatar.position; 
    options.position = position;
    options.volume = 0.5;
    Audio.playSound(sound, options);
}

function updateWarp() {
    if (!warpActive) return;

    var look = Quat.getFront(Camera.getOrientation());
    var deltaPosition = Vec3.subtract(MyAvatar.getTrackedHeadPosition(), headStartPosition);
    var deltaPitch = Math.abs(MyAvatar.getHeadFinalPitch() - headStartFinalPitch);
    deltaYaw = MyAvatar.getHeadFinalYaw() - headStartYaw;

    willMove = (!watchAvatar && (Math.abs(deltaYaw) < MAX_WARP_YAW) && (deltaPitch > WARP_PITCH_DEAD_ZONE));

    if (willMove) {
        var distance = Math.pow((deltaPitch - WARP_PITCH_DEAD_ZONE) * WARP_SENSITIVITY, 2.0);
        var warpDirection = Vec3.normalize({ x: look.x, y: 0, z: look.z });
        warpPosition = Vec3.multiply(warpDirection, distance);
        warpPosition = Vec3.sum(MyAvatar.position, warpPosition);
    }

    if (!watchAvatar && (Math.abs(deltaYaw) < MAX_PULLBACK_YAW) && (deltaPosition.z > TRIGGER_PULLBACK_DISTANCE)) {
        saveCameraState();
        var height = MyAvatar.getEyePosition().y - MyAvatar.position.y;
        var cameraPosition = Vec3.subtract(MyAvatar.position, Vec3.multiplyQbyV(Camera.getOrientation(), { x: 0, y: -height, z: -height * WATCH_AVATAR_DISTANCE }));
        Camera.setPosition(cameraPosition);
        watchAvatar = true;
    }

    // Adjust overlays to match warp position
    Overlays.editOverlay(warpSphere, {
        position: warpPosition,
        visible: willMove,
    });
    Overlays.editOverlay(warpLine, {
        position: Vec3.sum(warpPosition, { x: 0, y: -WARP_LINE_HEIGHT / 2.0, z: 0 }),
        end: Vec3.sum(warpPosition, { x: 0, y: WARP_LINE_HEIGHT / 2.0, z: 0 }),
        visible: willMove,
    });
}

function finishWarp() {
    if (!warpActive) return;
    warpActive = false;
    Overlays.editOverlay(warpSphere, {
        visible: false,
    });
    Overlays.editOverlay(warpLine, {
        visible: false,
    });
    if (willMove) {
        MyAvatar.position = warpPosition;
        playSound();
    } else if (!watchAvatar) {
        isTurning = true;
        turnAmount = deltaYaw;
        timeSinceLastTurn = TURN_INTERVAL;
        playSound();
    } 
}

var EPSILON_YAW = 180.0;
var TURN_INTERVAL = 0.175; 

function update(deltaTime) {

    if (movingWithHead) {
        keyDownTime += deltaTime;
        updateWarp();
    }

    if (isTurning) {
        timeSinceLastTurn += deltaTime; 
        if (timeSinceLastTurn > TURN_INTERVAL) {
            if (Math.abs(turnAmount) <= EPSILON_YAW) {
                var thisTurn = turnAmount;
                isTurning = false;
            } else {
                var thisTurn = turnAmount / 2.0;
                turnAmount = turnAmount / 2.0;
            }
            var orientation = MyAvatar.orientation;
            orientation = Quat.multiply(Quat.fromPitchYawRollDegrees(0, thisTurn, 0), orientation) ;
            MyAvatar.orientation = orientation;
            timeSinceLastTurn = 0.0;
        }
    }
}

Controller.keyPressEvent.connect(function(event) {
    if (event.text == "SPACE" && !movingWithHead) {
        keyDownTime = 0.0;
        movingWithHead = true;
        headStartPosition = MyAvatar.getTrackedHeadPosition();
        headStartDeltaPitch = MyAvatar.getHeadDeltaPitch();
        headStartFinalPitch = MyAvatar.getHeadFinalPitch();
        headStartRoll = MyAvatar.getHeadFinalRoll();
        headStartYaw = MyAvatar.getHeadFinalYaw();
        deltaYaw = 0.0;
        activateWarp();
    }
});

var TIME_FOR_TURNAROUND = 0.15;
var TURN_AROUND = 180.0;

Controller.keyReleaseEvent.connect(function(event) {
    if (event.text == "SPACE") {
        movingWithHead = false;
        isTurning = false;
        if (keyDownTime < TIME_FOR_TURNAROUND) {
            MyAvatar.orientation = Quat.multiply(Quat.fromPitchYawRollDegrees(0, TURN_AROUND, 0), MyAvatar.orientation);
            playSound();
        }
        finishWarp();
        if (watchAvatar) {
            restoreCameraState();
            watchAvatar = false; 
        }
    }
});

Script.update.connect(update);

