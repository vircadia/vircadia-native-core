//
//  gracefulControls.js
//  examples
//
//  Created by Ryan Huffman on 9/11/14
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

var DEFAULT_PARAMETERS = {
    // Coefficient to use for linear drag.  Higher numbers will cause motion to
    // slow down more quickly.
    DRAG_COEFFICIENT: 60.0,
    MAX_SPEED: 10.0,
    ACCELERATION: 10.0,

    MOUSE_YAW_SCALE: -0.125,
    MOUSE_PITCH_SCALE: -0.125,
    MOUSE_SENSITIVITY: 0.5,

    // Damping frequency, adjust to change mouse look behavior
    W: 2.2,
}

var BRAKE_PARAMETERS = {
    DRAG_COEFFICIENT: 4.9,
    MAX_SPEED: DEFAULT_PARAMETERS.MAX_SPEED,
    ACCELERATION: 0,

    W: 1.0,
    MOUSE_YAW_SCALE: -0.125,
    MOUSE_PITCH_SCALE: -0.125,
    MOUSE_SENSITIVITY: 0.5,
}

var DRIVE_AVATAR_ENABLED = true;
var UPDATE_RATE = 90;
var USE_INTERVAL = true;

var movementParameters = DEFAULT_PARAMETERS;


// Movement keys
var KEY_BRAKE = "Q";
var KEY_FORWARD = "W";
var KEY_BACKWARD = "S";
var KEY_LEFT = "A";
var KEY_RIGHT = "D";
var KEY_UP = "E";
var KEY_DOWN = "C";
var KEY_TOGGLE= "Space";

var KEYS;
if (DRIVE_AVATAR_ENABLED) {
    KEYS = [KEY_BRAKE, KEY_FORWARD, KEY_BACKWARD, KEY_LEFT, KEY_RIGHT, KEY_UP, KEY_DOWN];
} else {
    KEYS = [];
}

// Global Variables
var keys = {};
var velocity = { x: 0, y: 0, z: 0 };
var velocityVertical = 0;
var enabled = false;

var pos = Reticle.getPosition();
var lastX = pos.x;
var lastY = pos.y;
var yawFromMouse = 0;
var pitchFromMouse = 0;

var yawSpeed = 0;
var pitchSpeed = 0;


function update(dt) {
    if (enabled && Window.hasFocus()) {
        var pos = Reticle.getPosition();
        var x = pos.x;
        var y = pos.y;

        var dx = x - lastX;
        var dy = y - lastY;

        yawFromMouse += (dx * movementParameters.MOUSE_YAW_SCALE * movementParameters.MOUSE_SENSITIVITY);
        pitchFromMouse += (dy * movementParameters.MOUSE_PITCH_SCALE * movementParameters.MOUSE_SENSITIVITY);
        pitchFromMouse = Math.max(-180, Math.min(180, pitchFromMouse));

        resetCursorPosition();
    }

    // Here we use a linear damping model - http://en.wikipedia.org/wiki/Damping#Linear_damping
    // Because we are using a critically damped model (no oscillation), ζ = 1 and
    // so we derive the formula: acceleration = -(2 * w0 * v) - (w0^2 * x)
    var W = movementParameters.W;
    yawAccel = (W * W * yawFromMouse) - (2 * W * yawSpeed);
    pitchAccel = (W * W * pitchFromMouse) - (2 * W * pitchSpeed);

    yawSpeed += yawAccel * dt;
    var yawMove = yawSpeed * dt;
    var newOrientation = Quat.multiply(MyAvatar.orientation, Quat.fromVec3Degrees( { x: 0, y: yawMove, z: 0 } ));
    MyAvatar.orientation = newOrientation;
    yawFromMouse -= yawMove;

    pitchSpeed += pitchAccel * dt;
    var pitchMove = pitchSpeed * dt;
    var newPitch = MyAvatar.headPitch + pitchMove;
    MyAvatar.headPitch = newPitch;
    pitchFromMouse -= pitchMove;


    if (DRIVE_AVATAR_ENABLED) {
        var targetVelocity = { x: 0, y: 0, z: 0 };
        var targetVelocityVertical = 0;
        var acceleration = movementParameters.ACCELERATION;

        if (keys[KEY_FORWARD]) {
            targetVelocity.z -= acceleration * dt;
        }
        if (keys[KEY_LEFT]) {
            targetVelocity.x -= acceleration * dt;
        }
        if (keys[KEY_BACKWARD]) {
            targetVelocity.z += acceleration * dt;
        }
        if (keys[KEY_RIGHT]) {
            targetVelocity.x += acceleration * dt;
        }
        if (keys[KEY_UP]) {
            targetVelocityVertical += acceleration * dt;
        }
        if (keys[KEY_DOWN]) {
            targetVelocityVertical -= acceleration * dt;
        }

        // If force isn't being applied in a direction, add drag;
        var drag = Math.max(movementParameters.DRAG_COEFFICIENT * dt, 1.0);
        if (targetVelocity.x == 0) {
            targetVelocity.x = -velocity.x * drag;
        }
        if (targetVelocity.z == 0) {
            targetVelocity.z = -velocity.z * drag;
        }
        velocity = Vec3.sum(velocity, targetVelocity);

        var maxSpeed = movementParameters.MAX_SPEED;
        velocity.x = Math.max(-maxSpeed, Math.min(maxSpeed, velocity.x));
        velocity.z = Math.max(-maxSpeed, Math.min(maxSpeed, velocity.z));
        var v = Vec3.multiplyQbyV(MyAvatar.headOrientation, velocity);

        if (targetVelocityVertical == 0) {
            targetVelocityVertical -= (velocityVertical * movementParameters.DRAG_COEFFICIENT * dt);
        }
        velocityVertical += targetVelocityVertical;
        velocityVertical = Math.max(-maxSpeed, Math.min(maxSpeed, velocityVertical));
        v.y += velocityVertical;

        MyAvatar.motorVelocity = v;
    }
}

function vecToString(vec) {
    return vec.x + ", " + vec.y + ", " + vec.z;
}

function resetCursorPosition() {
    var newX = Math.floor(Window.innerWidth / 2);
    var newY = Math.floor(Window.innerHeight / 2);
    Reticle.setPosition({ x: newX, y: newY });
    lastX = newX;
    lastY = newY;
}


function toggleEnabled() {
    if (enabled) {
        disable();
    } else {
        enable();
    }
}


var timerID = null;
function enable() {
    if (!enabled && Window.hasFocus()) {
        enabled = true;

        resetCursorPosition();

        // Reset movement variables
        yawFromMouse = 0;
        pitchFromMouse = 0;
        yawSpeed = 0;
        pitchSpeed = 0;
        velocityVertical = 0;
        velocity = { x: 0, y: 0, z: 0 };

        MyAvatar.motorReferenceFrame = 'world';
        MyAvatar.motorVelocity = { x: 0, y: 0, z: 0 };
        MyAvatar.motorTimescale = 1;

        Controller.enableMapping(MAPPING_KEYS_NAME);

        Reticle.setVisible(false);
        if (USE_INTERVAL) {
            var lastTime = Date.now();
            timerID = Script.setInterval(function() {
                var now = Date.now();
                var dt = now - lastTime;
                lastTime = now;
                update(dt / 1000);
            }, (1.0 / UPDATE_RATE) * 1000);
        } else {
            Script.update.connect(update);
        }
    }
}

function disable() {
    if (enabled) {
        enabled = false;
        Reticle.setVisible(true);

        MyAvatar.motorVelocity = { x: 0, y: 0, z: 0 };

        Controller.disableMapping(MAPPING_KEYS_NAME);

        if (USE_INTERVAL) {
            Script.clearInterval(timerID);
            timerID = null;
        } else {
            Script.update.disconnect(update);
        }
    }
}

function scriptEnding() {
    disable();
    Controller.disableMapping(MAPPING_ENABLE_NAME);
    Controller.disableMapping(MAPPING_KEYS_NAME);
}


var MAPPING_ENABLE_NAME = 'io.highfidelity.gracefulControls.toggle';
var MAPPING_KEYS_NAME = 'io.highfidelity.gracefulControls.keys';
var keyControllerMapping = Controller.newMapping(MAPPING_KEYS_NAME);
var enableControllerMapping = Controller.newMapping(MAPPING_ENABLE_NAME);

function onKeyPress(key, value) {
    print(key, value);
    keys[key] = value > 0;

    if (value > 0) {
        if (key == KEY_TOGGLE) {
            toggleEnabled();
        } else if (key == KEY_BRAKE) {
            movementParameters = BRAKE_PARAMETERS;
        }
    } else {
        if (key == KEY_BRAKE) {
            movementParameters = DEFAULT_PARAMETERS;
        }
    }
}

for (var i = 0; i < KEYS.length; ++i) {
    var key = KEYS[i];
    var hw = Controller.Hardware.Keyboard[key];
    if (hw) {
        keyControllerMapping.from(hw).to(function(key) {
            return function(value) {
                onKeyPress(key, value);
            };
        }(key));
    } else {
        print("Unknown key: ", key);
    }
}

enableControllerMapping.from(Controller.Hardware.Keyboard[KEY_TOGGLE]).to(function(value) {
    onKeyPress(KEY_TOGGLE, value);
});

Controller.enableMapping(MAPPING_ENABLE_NAME);

Script.scriptEnding.connect(scriptEnding);
