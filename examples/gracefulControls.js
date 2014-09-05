// Coefficient to use for linear drag.  Higher numbers will cause motion to
// slow down more quickly.
/*
var DRAG_COEFFICIENT = 0.9;
var MAX_SPEED = 50.0;
var MAX_LOOK_SPEED = Math.PI * 2;
var ACCELERATION = 15;

var MOUSE_YAW_SCALE = -0.125;
var MOUSE_PITCH_SCALE = -0.125;
var MOUSE_SENSITIVITY = 0.5;

var W = 4.2;
*/

var DEFAULT_PARAMETERS = {
    DRAG_COEFFICIENT: 0.9,
    MAX_SPEED: 50.0,
    MAX_LOOK_SPEED: Math.PI * 2,
    ACCELERATION: 15,

    MOUSE_YAW_SCALE: -0.125,
    MOUSE_PITCH_SCALE: -0.125,
    MOUSE_SENSITIVITY: 0.5,

    W: 4.2,
}

var BRAKE_PARAMETERS = {
    DRAG_COEFFICIENT: 4.9,
    MAX_SPEED: DEFAULT_PARAMETERS.MAX_SPEED,
    MAX_LOOK_SPEED: Math.PI * 2,
    ACCELERATION: 15,

    MOUSE_YAW_SCALE: -0.125,
    MOUSE_PITCH_SCALE: -0.125,
    MOUSE_SENSITIVITY: 0.5,

    W: 1.0,
}

var movementParameters = DEFAULT_PARAMETERS;

// Movement keys
var KEY_BRAKE = "q";
var KEY_FORWARD = "w";
var KEY_BACKWARD = "s";
var KEY_LEFT = "a";
var KEY_RIGHT = "d";
var KEY_UP = "e";
var KEY_DOWN = "c";
var CAPTURED_KEYS = [KEY_FORWARD, KEY_BACKWARD, KEY_LEFT, KEY_RIGHT, KEY_UP, KEY_DOWN];

// Global Variables
var keys = {};
var velocity = { x: 0, y: 0, z: 0 };
var velocityVertical = 0;
var enabled = false;

var lastX = Window.getCursorPositionX();
var lastY = Window.getCursorPositionY();
var yawFromMouse = 0;
var pitchFromMouse = 0;

var yawSpeed = 0;
var pitchSpeed = 0;

function keyPressEvent(event) {
    if (event.text == "ESC") {
        disable();
    } else if (event.text == "SPACE") {
        if (Window.hasFocus()) {
            enable();
        }
    } else if (event.text == KEY_BRAKE) {
        movementParameters = BRAKE_PARAMETERS;
    }
    keys[event.text] = true;
}

function keyReleaseEvent(event) {
    if (event.text == KEY_BRAKE) {
        movementParameters = DEFAULT_PARAMETERS;
    }

    delete keys[event.text];
}

function update(dt) {
    var maxMove = 3.0 * dt;
    // print("Pos: " + yawFromMouse + ", " + pitchFromMouse);
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

    if (enabled && Window.hasFocus()) {
        var x = Window.getCursorPositionX();
        var y = Window.getCursorPositionY();

        yawFromMouse += ((x - lastX) * movementParameters.MOUSE_YAW_SCALE * movementParameters.MOUSE_SENSITIVITY);
        pitchFromMouse += ((y - lastY) * movementParameters.MOUSE_PITCH_SCALE * movementParameters.MOUSE_SENSITIVITY);
        pitchFromMouse = Math.max(-180, Math.min(180, pitchFromMouse));

        var newX = Window.innerWidth / 2;
        var newY = Window.innerHeight / 2;
        Window.setCursorPosition(newX, newY);

        lastX = newX;
        lastY = newY;
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

    // If force isn't being applied in a direction, add drag;
    if (targetVelocity.x == 0) {
        targetVelocity.x -= (velocity.x * movementParameters.DRAG_COEFFICIENT * dt);
    }
    if (targetVelocity.z == 0) {
        targetVelocity.z -= (velocity.z * movementParameters.DRAG_COEFFICIENT * dt);
    }
    velocity = Vec3.sum(velocity, targetVelocity);

    var maxSpeed = movementParameters.MAX_SPEED;
    velocity.x = Math.max(-maxSpeed, Math.min(maxSpeed, velocity.x));
    velocity.z = Math.max(-maxSpeed, Math.min(maxSpeed, velocity.z));
    var v = Quat.rotate(MyAvatar.headOrientation, velocity);

    if (velocityVertical == 0) {
        targetVelocityVertical -= (velocityVertical * movementParameters.DRAG_COEFFICIENT * dt);
    }
    velocityVertical += targetVelocityVertical;
    velocityVertical = Math.max(-maxSpeed, Math.min(maxSpeed, velocityVertical));
    v.y += velocityVertical;

    MyAvatar.setVelocity(v);
}

function vecToString(vec) {
    return vec.x + ", " + vec.y + ", " + vec.z;
}

function scriptEnding() {
    disable();
}

function enable() {
    if (!enabled) {
        enabled = true;
        lastX = Window.getCursorPositionX();
        lastY = Window.getCursorPositionY();
        for (var i = 0; i < CAPTURED_KEYS.length; i++) {
            Controller.captureKeyEvents({ text: CAPTURED_KEYS[i] });
        }
        Window.setCursorVisible(false);
        Script.update.connect(update);
    }
}

function disable() {
    if (enabled) {
        enabled = false;
        for (var i = 0; i < CAPTURED_KEYS.length; i++) {
            Controller.releaseKeyEvents({ text: CAPTURED_KEYS[i] });
        }
        Window.setCursorVisible(true);
        //Script.update.disconnect(update);
    }
}

Controller.keyPressEvent.connect(keyPressEvent);
Controller.keyReleaseEvent.connect(keyReleaseEvent);

Script.scriptEnding.connect(scriptEnding);
