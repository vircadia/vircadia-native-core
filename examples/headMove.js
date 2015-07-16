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

var gamepads = {};

var debug = false;
var willMove = false;

var warpActive = false;
var warpPosition = { x: 0, y: 0, z: 0 };

var hipsToEyes;
var restoreCountdownTimer;

//  Overlays to show target location 

var WARP_SPHERE_SIZE = 0.085;
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
    start: { x: 0, y: 0, z:0 },
    end: { x: 0, y: 0, z: 0 },
    color: { red: 0, green: 255, blue: 255},
    alpha: 1,
    lineWidth: 5,
    visible: false,
});

var movingWithHead = false; 
var headStartPosition, headStartDeltaPitch, headStartFinalPitch, headStartRoll, headStartYaw;
var deltaYaw = 0.0;
var keyDownTime = 0.0;
var timeSinceLastUp = 0.0;
var watchAvatar = false; 
var oldMode; 
var lastYawTurned = 0.0;
var startPullbackPosition;

function saveCameraState() {
    oldMode = Camera.mode;
    Camera.mode = "independent";
}

function restoreCameraState() {
    Camera.mode = oldMode;
}

var WATCH_AVATAR_DISTANCE = 2.5;

var sound = SoundCache.getSound("http://public.highfidelity.io/sounds/Footsteps/FootstepW2Right-12db.wav");
function playSound() {
    Audio.playSound(sound, {
      position: MyAvatar.position
    });
}

function pullBack() {
    saveCameraState();
    cameraPosition = Vec3.subtract(MyAvatar.position, Vec3.multiplyQbyV(Camera.getOrientation(), { x: 0, y: -hipsToEyes, z: -hipsToEyes * WATCH_AVATAR_DISTANCE }));
    Camera.setPosition(cameraPosition);
    cameraPosition = Camera.getPosition();
    startPullbackPosition = cameraPosition;
}

var WARP_SMOOTHING = 0.90;
var WARP_START_TIME = 0.25;
var WARP_START_DISTANCE = 2.5;
var WARP_SENSITIVITY = 0.15;
var MAX_WARP_DISTANCE = 25.0;

var fixedHeight = true;

function updateWarp() {
    if (!warpActive) return;

    var viewEulers = Quat.safeEulerAngles(Camera.getOrientation());
    var deltaPosition = Vec3.subtract(MyAvatar.getTrackedHeadPosition(), headStartPosition);
    var deltaPitch = MyAvatar.getHeadFinalPitch() - headStartFinalPitch;
    deltaYaw = MyAvatar.getHeadFinalYaw() - headStartYaw;
    viewEulers.x -= deltaPitch;
    var look = Quat.getFront(Quat.fromVec3Degrees(viewEulers));

    willMove = (keyDownTime > WARP_START_TIME);

    if (willMove) {
        var distance = Math.min(Math.exp(deltaPitch * WARP_SENSITIVITY) * WARP_START_DISTANCE, MAX_WARP_DISTANCE);
        var warpDirection = Vec3.normalize({ x: look.x, y: (fixedHeight ? 0 : look.y), z: look.z });
        var startPosition = (watchAvatar ? Camera.getPosition(): MyAvatar.getEyePosition());
        warpPosition = Vec3.mix(Vec3.sum(startPosition, Vec3.multiply(warpDirection, distance)), warpPosition, WARP_SMOOTHING);
    }

    var cameraPosition;

    if (!watchAvatar && willMove && (distance < WARP_START_DISTANCE * 0.5)) {
        pullBack();
        watchAvatar = true;
    }

    // Adjust overlays to match warp position
    Overlays.editOverlay(warpSphere, {
        position: warpPosition,
        visible: willMove,
    });
    Overlays.editOverlay(warpLine, {
        start: Vec3.sum(warpPosition, { x: 0, y: -WARP_LINE_HEIGHT / 2.0, z: 0 }),
        end: Vec3.sum(warpPosition, { x: 0, y: WARP_LINE_HEIGHT / 2.0, z: 0 }),
        visible: willMove,
    });
}

function activateWarp() {
    if (warpActive) return;
    warpActive = true;
    movingWithHead = true;
    hipsToEyes = MyAvatar.getEyePosition().y - MyAvatar.position.y;
    headStartPosition = MyAvatar.getTrackedHeadPosition();
    headStartDeltaPitch = MyAvatar.getHeadDeltaPitch();
    headStartFinalPitch = MyAvatar.getHeadFinalPitch();
    headStartRoll = MyAvatar.getHeadFinalRoll();
    headStartYaw = MyAvatar.getHeadFinalYaw();
    deltaYaw = 0.0;
    warpPosition = MyAvatar.position;
    warpPosition.y += hipsToEyes; 
    updateWarp();
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
        if (fixedHeight) {
            warpPosition.y = MyAvatar.position.y;
        } else {
            warpPosition.y -= hipsToEyes;
        }
        
        MyAvatar.position = warpPosition;
        cameraPosition = Vec3.subtract(MyAvatar.position, Vec3.multiplyQbyV(Camera.getOrientation(), { x: 0, y: -hipsToEyes, z: -hipsToEyes * WATCH_AVATAR_DISTANCE }));
        Camera.setPosition(cameraPosition);
        playSound();
        if (watchAvatar) {
            restoreCountdownTimer = RESTORE_TIME;
        }
    } 
}

function update(deltaTime) {
    timeSinceLastUp += deltaTime;
    if (movingWithHead) {
        keyDownTime += deltaTime;
        updateWarp();
    }
    if (restoreCountdownTimer > 0.0) {
        restoreCountdownTimer -= deltaTime; 
        if (restoreCountdownTimer <= 0.0) {
            restoreCameraState();
            watchAvatar = false; 
            restoreCountDownTimer = 0.0;
        }
    }
}

Controller.keyPressEvent.connect(function(event) {
    if (event.text == "SPACE" && !event.isAutoRepeat && !movingWithHead) {
        keyDownTime = 0.0;
        activateWarp();
    }
});
                                 
var DOUBLE_CLICK_TIME = 0.50;
var TURN_AROUND = 180.0;
var RESTORE_TIME = 0.50;

Controller.keyReleaseEvent.connect(function(event) {
    if (event.text == "SPACE" && !event.isAutoRepeat) {
        movingWithHead = false;
        if (timeSinceLastUp < DOUBLE_CLICK_TIME) {
            // Turn all the way around 
            var turnRemaining = TURN_AROUND - lastYawTurned;
            lastYawTurned = 0.0;
            MyAvatar.orientation = Quat.multiply(Quat.fromPitchYawRollDegrees(0, TURN_AROUND, 0), MyAvatar.orientation);
            playSound();
        } else if (keyDownTime < WARP_START_TIME) {
            var currentYaw = MyAvatar.getHeadFinalYaw();
            lastYawTurned = currentYaw;
            MyAvatar.orientation = Quat.multiply(Quat.fromPitchYawRollDegrees(0, currentYaw, 0), MyAvatar.orientation);
            playSound();
        }
        timeSinceLastUp = 0.0;
        finishWarp();
    }
});

function reportButtonValue(button, newValue, oldValue) {
    if (button == Joysticks.BUTTON_FACE_RIGHT) {
        if (newValue) {
            activateWarp();
        } else {
            finishWarp();
        }
    }
}

Script.update.connect(update);

function addJoystick(gamepad) {
    gamepad.buttonStateChanged.connect(reportButtonValue);

    gamepads[gamepad.instanceId] = gamepad;

    print("Added gamepad: " + gamepad.name + " (" + gamepad.instanceId + ")");
}

function removeJoystick(gamepad) {
    delete gamepads[gamepad.instanceId]

    print("Removed gamepad: " + gamepad.name + " (" + gamepad.instanceId + ")");
}

var allJoysticks = Joysticks.getAllJoysticks();
for (var i = 0; i < allJoysticks.length; i++) {
    addJoystick(allJoysticks[i]);
}

Joysticks.joystickAdded.connect(addJoystick);
Joysticks.joystickRemoved.connect(removeJoystick);

