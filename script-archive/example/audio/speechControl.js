//
//  speechControl.js
//  examples
//
//  Created by Ryan Huffman on 07/31/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

var ACCELERATION = 80;
var STEP_DURATION = 1.0; // Duration of a step command in seconds
var TURN_DEGREES = 90;
var SLIGHT_TURN_DEGREES = 45;
var TURN_AROUND_DEGREES = 180;
var TURN_RATE = 90; // Turn rate in degrees per second


/*****************************************************************************/
/** COMMANDS *****************************************************************/
var CMD_MOVE_FORWARD = "Move forward";
var CMD_MOVE_BACKWARD = "Move backward";
var CMD_MOVE_UP = "Move up";
var CMD_MOVE_DOWN = "Move down";
var CMD_MOVE_LEFT = "Move left";
var CMD_MOVE_RIGHT = "Move right";

var CMD_STEP_FORWARD = "Step forward";
var CMD_STEP_BACKWARD = "Step backward";
var CMD_STEP_LEFT = "Step left";
var CMD_STEP_RIGHT = "Step right";
var CMD_STEP_UP = "Step up";
var CMD_STEP_DOWN = "Step down";

var CMD_TURN_LEFT = "Turn left";
var CMD_TURN_SLIGHT_LEFT = "Turn slight left";
var CMD_TURN_RIGHT = "Turn right";
var CMD_TURN_SLIGHT_RIGHT = "Turn slight right";
var CMD_TURN_AROUND = "Turn around";

var CMD_STOP = "Stop";

var CMD_SHOW_COMMANDS = "Show commands";

var MOVE_COMMANDS = [
    CMD_MOVE_FORWARD,
    CMD_MOVE_BACKWARD,
    CMD_MOVE_UP,
    CMD_MOVE_DOWN,
    CMD_MOVE_LEFT,
    CMD_MOVE_RIGHT,
];

var STEP_COMMANDS = [
    CMD_STEP_FORWARD,
    CMD_STEP_BACKWARD,
    CMD_STEP_UP,
    CMD_STEP_DOWN,
    CMD_STEP_LEFT,
    CMD_STEP_RIGHT,
];

var TURN_COMMANDS = [
    CMD_TURN_LEFT,
    CMD_TURN_SLIGHT_LEFT,
    CMD_TURN_RIGHT,
    CMD_TURN_SLIGHT_RIGHT,
    CMD_TURN_AROUND,
];

var OTHER_COMMANDS = [
    CMD_STOP,
    CMD_SHOW_COMMANDS,
];

var ALL_COMMANDS = []
    .concat(MOVE_COMMANDS)
    .concat(STEP_COMMANDS)
    .concat(TURN_COMMANDS)
    .concat(OTHER_COMMANDS);

/** END OF COMMANDS **********************************************************/
/*****************************************************************************/


var currentCommandFunc = null;

function handleCommandRecognized(command) {
    if (MOVE_COMMANDS.indexOf(command) > -1 || STEP_COMMANDS.indexOf(command) > -1) {
        // If this is a STEP_* command, we will want to countdown the duration
        // of time to move.  MOVE_* commands don't stop.
        var timeRemaining = MOVE_COMMANDS.indexOf(command) > -1 ? 0 : STEP_DURATION;
        var accel = { x: 0, y: 0, z: 0 };

        if (command == CMD_MOVE_FORWARD || command == CMD_STEP_FORWARD) {
            accel = { x: 0, y: 0, z: 1 };
        } else if (command == CMD_MOVE_BACKWARD || command == CMD_STEP_BACKWARD) {
            accel = { x: 0, y: 0, z: -1 };
        } else if (command === CMD_MOVE_UP || command == CMD_STEP_UP) {
            accel = { x: 0, y: 1, z: 0 };
        } else if (command == CMD_MOVE_DOWN || command == CMD_STEP_DOWN) {
            accel = { x: 0, y: -1, z: 0 };
        } else if (command == CMD_MOVE_LEFT || command == CMD_STEP_LEFT) {
            accel = { x: -1, y: 0, z: 0 };
        } else if (command == CMD_MOVE_RIGHT || command == CMD_STEP_RIGHT) {
            accel = { x: 1, y: 0, z: 0 };
        }

        currentCommandFunc = function(dt) {
            if (timeRemaining > 0 && dt >= timeRemaining) {
                dt = timeRemaining;
            }

            var headOrientation = MyAvatar.headOrientation;
            var front = Quat.getFront(headOrientation);
            var right = Quat.getRight(headOrientation);
            var up = Quat.getUp(headOrientation);

            var thrust = Vec3.multiply(front, accel.z * ACCELERATION);
            thrust = Vec3.sum(thrust, Vec3.multiply(right, accel.x * ACCELERATION));
            thrust = Vec3.sum(thrust, Vec3.multiply(up, accel.y * ACCELERATION));
            MyAvatar.addThrust(thrust);

            if (timeRemaining > 0) {
                timeRemaining -= dt;
                return timeRemaining > 0;
            }

            return true;
        };
    } else if (TURN_COMMANDS.indexOf(command) > -1) {
        var degreesRemaining;
        var sign;
        if (command == CMD_TURN_LEFT) {
            sign = 1;
            degreesRemaining = TURN_DEGREES;
        } else if (command == CMD_TURN_RIGHT) {
            sign = -1;
            degreesRemaining = TURN_DEGREES;
        } else if (command == CMD_TURN_SLIGHT_LEFT) {
            sign = 1;
            degreesRemaining = SLIGHT_TURN_DEGREES;
        } else if (command == CMD_TURN_SLIGHT_RIGHT) {
            sign = -1;
            degreesRemaining = SLIGHT_TURN_DEGREES;
        } else if (command == CMD_TURN_AROUND) {
            sign = 1;
            degreesRemaining = TURN_AROUND_DEGREES;
        }
        currentCommandFunc = function(dt) {
            // Determine how much to turn by
            var turnAmount = TURN_RATE * dt;
            if (turnAmount > degreesRemaining) {
                turnAmount = degreesRemaining;
            }

            // Apply turn
            var orientation = MyAvatar.orientation;
            var deltaOrientation = Quat.fromPitchYawRollDegrees(0, sign * turnAmount, 0);
            MyAvatar.orientation = Quat.multiply(orientation, deltaOrientation);

            degreesRemaining -= turnAmount;
            return turnAmount > 0;
        }
    } else if (command == CMD_STOP) {
        currentCommandFunc = null;
    } else if (command == CMD_SHOW_COMMANDS) {
        var msg = "";
        for (var i = 0; i < ALL_COMMANDS.length; i++) {
            msg += ALL_COMMANDS[i] + "\n";
        }
        Window.alert(msg);
    }
}

function update(dt) {
    if (currentCommandFunc) {
        if (currentCommandFunc(dt) === false) {
            currentCommandFunc = null;
        }
    }
}

function setup() {
    for (var i = 0; i < ALL_COMMANDS.length; i++) {
        SpeechRecognizer.addCommand(ALL_COMMANDS[i]);
    }
}

function scriptEnding() {
    for (var i = 0; i < ALL_COMMANDS.length; i++) {
        SpeechRecognizer.removeCommand(ALL_COMMANDS[i]);
    }
}

Script.scriptEnding.connect(scriptEnding);
Script.update.connect(update);
SpeechRecognizer.commandRecognized.connect(handleCommandRecognized);

setup();
