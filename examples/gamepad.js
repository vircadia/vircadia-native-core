//
//  controller.js
//  examples
//
//  Created by Ryan Huffman on 10/9/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

// TODO Update to work with any controller that is plugged in.
var CONTROLLER_NAMES = [
    "Wireless 360 Controller",
    "Controller (XBOX 360 For Windows)",
    "Controller", // Wired 360 controller
]

for (var i = 0; i < CONTROLLER_NAMES.length; i++) {
    gamepad = Joysticks.joystickWithName(CONTROLLER_NAMES[i]);
    if (gamepad) {
        print("Found controller: " + CONTROLLER_NAMES[i]);
        break;
    }
}

if (!gamepad) {
    print("No gamepad found.");
}

// Controller axis/button mappings
var GAMEPAD = {
    AXES: {
        LEFT_JOYSTICK_X: 0,
        LEFT_JOYSTICK_Y: 1,

        RIGHT_JOYSTICK_X: 2,
        RIGHT_JOYSTICK_Y: 3,

        LEFT_TRIGGER: 4,
        RIGHT_TRIGGER: 5,
    },
    BUTTONS: {
        DPAD_UP: 0,
        DPAD_DOWN: 1,
        DPAD_LEFT: 2,
        DPAD_RIGHT: 3,

        LEFT_JOYSTICK: 6,
        RIGHT_JOYSTICK: 7,

        LEFT_BUMPER: 8,
        RIGHT_BUMPER: 9,

        // Face buttons, ABXY on an XBOX controller
        FACE_BOTTOM: 11,
        FACE_RIGHT: 12,
        FACE_LEFT: 13,
        FACE_TOP: 14,
    }
}

// Button/axis mappings
var AXIS_STRAFE = GAMEPAD.AXES.LEFT_JOYSTICK_X;
var AXIS_FORWARD = GAMEPAD.AXES.LEFT_JOYSTICK_Y;
var AXIS_ROTATE = GAMEPAD.AXES.RIGHT_JOYSTICK_X;

var BUTTON_TURN_AROUND = GAMEPAD.BUTTONS.RIGHT_JOYSTICK;

var BUTTON_FLY_UP = GAMEPAD.BUTTONS.RIGHT_BUMPER;
var BUTTON_FLY_DOWN = GAMEPAD.BUTTONS.LEFT_BUMPER
var BUTTON_WARP = GAMEPAD.BUTTONS.FACE_BOTTOM;

var BUTTON_WARP_FORWARD = GAMEPAD.BUTTONS.DPAD_UP;
var BUTTON_WARP_BACKWARD = GAMEPAD.BUTTONS.DPAD_DOWN;
var BUTTON_WARP_LEFT = GAMEPAD.BUTTONS.DPAD_LEFT;
var BUTTON_WARP_RIGHT = GAMEPAD.BUTTONS.DPAD_RIGHT;

// Distance in meters to warp via BUTTON_WARP_*
var WARP_DISTANCE = 1;

// Walk speed in m/s
var MOVE_SPEED = 2;

// Amount to rotate in radians
var ROTATE_INCREMENT = Math.PI / 8;

// Pick from above where we want to warp
var WARP_PICK_OFFSET = { x: 0, y: 10, z: 0 };

// When warping, the warp position will snap to a target below the current warp position.
// This is the max distance it will snap to.
var WARP_PICK_MAX_DISTANCE = 100;

var flyDownButtonState = false;
var flyUpButtonState = false;

// Current move direction, axis aligned - that is, looking down and moving forward
// will not move you into the ground, but instead will keep you on the horizontal plane.
var moveDirection = { x: 0, y: 0, z: 0 };

var warpActive = false;
var warpPosition = { x: 0, y: 0, z: 0 };

var WARP_SPHERE_SIZE = 1;
var warpSphere = Overlays.addOverlay("sphere", {
    position: { x: 0, y: 0, z: 0 },
    size: WARP_SPHERE_SIZE,
    color: { red: 0, green: 255, blue: 0 },
    alpha: 1.0,
    solid: true,
    visible: false,
});

var WARP_LINE_HEIGHT = 10;
var warpLine = Overlays.addOverlay("line3d", {
    position: { x: 0, y: 0, z:0 },
    end: { x: 0, y: 0, z: 0 },
    color: { red: 0, green: 255, blue: 255},
    alpha: 1,
    lineWidth: 5,
    visible: false,
});

function copyVec3(vec) {
    return { x: vec.x, y: vec.y, z: vec.z };
}

function activateWarp() {
    if (warpActive) return;
    warpActive = true;

    updateWarp();
}

function updateWarp() {
    if (!warpActive) return;

    var look = Quat.getFront(Camera.getOrientation());
    var pitch = Math.asin(look.y);

    // Get relative to looking straight down
    pitch += Math.PI / 2;

    // Scale up
    pitch *= 2;
    var distance = pitch * pitch * pitch;

    var warpDirection = Vec3.normalize({ x: look.x, y: 0, z: look.z });
    warpPosition = Vec3.multiply(warpDirection, distance);
    warpPosition = Vec3.sum(MyAvatar.position, warpPosition);

    var pickRay = {
        origin: Vec3.sum(warpPosition, WARP_PICK_OFFSET),
        direction: { x: 0, y: -1, z: 0 }
    };

    var intersection = Voxels.findRayIntersection(pickRay);

    if (intersection.intersects && intersection.distance < WARP_PICK_MAX_DISTANCE) {
        // Warp 1 meter above the object - this is an approximation
        // TODO Get the actual offset to the Avatar's feet and plant them to
        // the object.
        warpPosition = Vec3.sum(intersection.intersection, { x: 0, y: 1, z:0 });
    }

    // Adjust overlays to match warp position
    Overlays.editOverlay(warpSphere, {
        position: warpPosition,
        visible: true,
    });
    Overlays.editOverlay(warpLine, {
        position: warpPosition,
        end: Vec3.sum(warpPosition, { x: 0, y: WARP_LINE_HEIGHT, z: 0 }),
        visible: true,
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
    MyAvatar.position = warpPosition;
}

function reportAxisValue(axis, newValue, oldValue) {
    if (Math.abs(oldValue) < 0.2) oldValue = 0;
    if (Math.abs(newValue) < 0.2) newValue = 0;

    if (axis == AXIS_FORWARD) {
        moveDirection.z = newValue;
    } else if (axis == AXIS_STRAFE) {
        moveDirection.x = newValue;
    } else if (axis == AXIS_ROTATE) {
        if (oldValue == 0 && newValue != 0) {
            var rotateRadians = newValue > 0 ? -ROTATE_INCREMENT : ROTATE_INCREMENT;
            var orientation = MyAvatar.orientation;
            orientation = Quat.multiply(Quat.fromPitchYawRollRadians(0, rotateRadians, 0), orientation) ;
            MyAvatar.orientation = orientation;
        }
    }
}

function reportButtonValue(button, newValue, oldValue) {
    if (button == BUTTON_FLY_DOWN) {
        flyDownButtonState = newValue;
    } else if (button == BUTTON_FLY_UP) {
        flyUpButtonState = newValue;
    } else if (button == BUTTON_WARP) {
        if (newValue) {
            activateWarp();
        } else {
            finishWarp();
        }
    } else if (button == BUTTON_TURN_AROUND) {
        if (newValue) {
            MyAvatar.orientation = Quat.multiply(
                Quat.fromPitchYawRollRadians(0, Math.PI, 0), MyAvatar.orientation);
        }
    } else if (newValue) {
        var direction = null;

        if (button == BUTTON_WARP_FORWARD) {
            direction = Quat.getFront(Camera.getOrientation());
        } else if (button == BUTTON_WARP_BACKWARD) {
            direction = Quat.getFront(Camera.getOrientation());
            direction = Vec3.multiply(-1, direction);
        } else if (button == BUTTON_WARP_LEFT) {
            direction = Quat.getRight(Camera.getOrientation());
            direction = Vec3.multiply(-1, direction);
        } else if (button == BUTTON_WARP_RIGHT) {
            direction = Quat.getRight(Camera.getOrientation());
        }

        if (direction) {
            direction.y = 0;
            direction = Vec3.multiply(Vec3.normalize(direction), WARP_DISTANCE);
            MyAvatar.position = Vec3.sum(MyAvatar.position, direction);
        }
    }

    if (flyUpButtonState && !flyDownButtonState) {
        moveDirection.y = 1;
    } else if (!flyUpButtonState && flyDownButtonState) {
        moveDirection.y = -1;
    } else {
        moveDirection.y = 0;
    }
}

function update(dt) {
    var velocity = { x: 0, y: 0, z: 0 };
    var move = copyVec3(moveDirection);
    move.y = 0;
    if (Vec3.length(move) > 0) {
        velocity = Vec3.multiplyQbyV(Camera.getOrientation(), move);
        velocity.y = 0;
        velocity = Vec3.multiply(Vec3.normalize(velocity), MOVE_SPEED);
    }

    if (moveDirection.y != 0) {
        velocity.y = moveDirection.y * MOVE_SPEED;
    }

    MyAvatar.setVelocity(velocity);

    updateWarp();
}

if (gamepad) {
    gamepad.axisValueChanged.connect(reportAxisValue);
    gamepad.buttonStateChanged.connect(reportButtonValue);

    Script.update.connect(update);
}
