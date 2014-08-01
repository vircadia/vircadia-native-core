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

var CMD_MOVE_FORWARD = "Move forward";
var CMD_MOVE_BACKWARD = "Move backward";
var CMD_MOVE_UP = "Move up";
var CMD_MOVE_DOWN = "Move down";
var CMD_STRAFE_LEFT = "Strafe left";
var CMD_STRAFE_RIGHT = "Strafe right";
var CMD_STOP = "Stop";

var CMD_SHOW_COMMANDS = "Show commands";

var commands = [
    CMD_MOVE_FORWARD,
    CMD_MOVE_BACKWARD,
    CMD_MOVE_UP,
    CMD_MOVE_DOWN,
    CMD_STRAFE_LEFT,
    CMD_STRAFE_RIGHT,
    CMD_STOP,
    CMD_SHOW_COMMANDS,
];

var moveForward = 0;
var moveRight = 0;
var turnRight = 0;
var moveUp = 0;

function commandRecognized(command) {
    if (command === CMD_MOVE_FORWARD) {
        accel = { x: 0, y: 0, z: 1 };
    } else if (command == CMD_MOVE_BACKWARD) {
        accel = { x: 0, y: 0, z: -1 };
    } else if (command === CMD_MOVE_UP) {
        accel = { x: 0, y: 1, z: 0 };
    } else if (command == CMD_MOVE_DOWN) {
        accel = { x: 0, y: -1, z: 0 };
    } else if (command == CMD_STRAFE_LEFT) {
        accel = { x: -1, y: 0, z: 0 };
    } else if (command == CMD_STRAFE_RIGHT) {
        accel = { x: 1, y: 0, z: 0 };
    } else if (command == CMD_STOP) {
        accel = { x: 0, y: 0, z: 0 };
    } else if (command == CMD_SHOW_COMMANDS) {
        var msg = "";
        for (var i = 0; i < commands.length; i++) {
            msg += commands[i] + "\n";
        }
        Window.alert(msg);
    }
}

var accel = { x: 0, y: 0, z: 0 };

var ACCELERATION = 80;
var initialized = false;

function update(dt) {
    var headOrientation = MyAvatar.headOrientation;
    var front = Quat.getFront(headOrientation);
    var right = Quat.getRight(headOrientation);
    var up = Quat.getUp(headOrientation);

    var thrust = Vec3.multiply(front, accel.z * ACCELERATION);
    thrust = Vec3.sum(thrust, Vec3.multiply(right, accel.x * ACCELERATION));
    thrust = Vec3.sum(thrust, Vec3.multiply(up, accel.y * ACCELERATION));
    // print(thrust.x + ", " + thrust.y + ", " + thrust.z);
    MyAvatar.addThrust(thrust);
}

function setup() {
    for (var i = 0; i < commands.length; i++) {
        SpeechRecognizer.addCommand(commands[i]);
    }
    print("SETTING UP");
}

function scriptEnding() {
    print("ENDING");
    for (var i = 0; i < commands.length; i++) {
        SpeechRecognizer.removeCommand(commands[i]);
    }
}

Script.scriptEnding.connect(scriptEnding);
Script.update.connect(update);
SpeechRecognizer.commandRecognized.connect(commandRecognized);

setup();
