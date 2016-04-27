//
//  move.js
//
//  Created by AndrewMeadows, 2014.09.17
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
//
// The avatar can be controlled by setting two motor parameters: motorVelocity and motorTimescale.
// Once the motorVelocity is set the avatar will try to move in that direction and speed.  The
// motorTimescale is the approximate amount of time it takes for the avatar to reach within 1/e of its
// motorVelocity, so a short timescale makes it ramp up fast, and a long timescale makes it slow.


// These parameters control the motor's speed and strength.
var MAX_MOTOR_TIMESCALE = 0.5;
var PUSHING_MOTOR_TIMESCALE = 0.25;
var BRAKING_MOTOR_TIMESCALE = 0.125;
var VERY_LONG_TIME = 1000000.0;

var AVATAR_SPEED = 4.0;
var MIN_BRAKING_SPEED = 0.2;


var motorAccumulator = {x:0.0, y:0.0, z:0.0};
var isBraking = false;

// There is a bug in Qt-5.3.0 (and below) that prevents QKeyEvent.isAutoRepeat from being correctly set.
// This means we can't tell when a held button is actually released -- all we get are the repeated 
// keyPress- (and keyRelease-) events.  So what we have to do is keep a list of last press timestamps
// for buttons of interest, and then periodically scan that list for old timestamps and drop any that
// have expired (at which point we actually remove that buttons effect on the motor).  As long as the 
// check period is longer than the time between key repeats then things will be smooth, and as long
// as the expiry time is short enough then the stop won't feel too laggy.

var MAX_AUTO_REPEAT_DELAY = 3;
var KEY_RELEASE_EXPIRY_MSEC = 100;

// KeyInfo class contructor:
function KeyInfo(contribution) {
    this.motorContribution = contribution; // Vec3 contribution of this key toward motorVelocity
    this.releaseTime = new Date(); // time when this button was released
    this.pressTime = new Date(); // time when this button was pressed
    this.isPressed = false;
}

// NOTE: the avatar's default orientation is such that "forward" is along the -zAxis, and "left" is along -xAxis.
var controlKeys = { 
    "UP"   : new KeyInfo({x: 0.0, y: 0.0, z:-1.0}),
    "DOWN" : new KeyInfo({x: 0.0, y: 0.0, z: 1.0}),
    "SHIFT+LEFT" : new KeyInfo({x:-1.0, y: 0.0, z: 0.0}),
    "SHIFT+RIGHT": new KeyInfo({x: 1.0, y: 0.0, z: 0.0}),
    "w" : new KeyInfo({x: 0.0, y: 0.0, z:-1.0}),
    "s" : new KeyInfo({x: 0.0, y: 0.0, z: 1.0}),
    "e" : new KeyInfo({x: 0.0, y: 1.0, z: 0.0}),
    "c" : new KeyInfo({x: 0.0, y:-1.0, z: 0.0})
};

// list of last timestamps when various buttons were last pressed
var pressTimestamps = {};

function keyPressEvent(event) {
    // NOTE: we're harvesting some of the same keyboard controls that are used by the default (scriptless) 
    // avatar control.  The scriptless control can be disabled via the Menu, thereby allowing this script 
    // to be the ONLY controller of the avatar position.
    
    var keyName = event.text;
    if (event.isShifted) {
        keyName = "SHIFT+" + keyName;
    }
  
    var key = controlKeys[keyName];
    if (key != undefined) {
        key.pressTime = new Date();
        // set the last pressTimestap element to undefined (MUCH faster than removing from the list)
        pressTimestamps[keyName] = undefined;
        var msec = key.pressTime.valueOf() - key.releaseTime.valueOf();
        if (!key.isPressed) {
            // add this key's effect to the motorAccumulator
            motorAccumulator = Vec3.sum(motorAccumulator, key.motorContribution);
            key.isPressed = true;
        }
    }
}

function keyReleaseEvent(event) {
    var keyName = event.text;
    if (event.isShifted) {
        keyName = "SHIFT+" + keyName;
    }

    var key = controlKeys[keyName];
    if (key != undefined) {
        // add key to pressTimestamps
        pressTimestamps[keyName] = new Date();
        key.releaseTime = new Date();
        var msec = key.releaseTime.valueOf() - key.pressTime.valueOf();
    }
}

function updateMotor(deltaTime) {
    // remove expired pressTimestamps
    var now = new Date();
    for (var keyName in pressTimestamps) {
        var t = pressTimestamps[keyName];
        if (t != undefined) {
            var msec = now.valueOf() - t.valueOf();
            if (msec > KEY_RELEASE_EXPIRY_MSEC) {
                // the release of this key is now official, and we remove it from the motorAccumulator
                motorAccumulator = Vec3.subtract(motorAccumulator, controlKeys[keyName].motorContribution);
                controlKeys[keyName].isPressed = false;
                // set the last pressTimestap element to undefined (MUCH faster than removing from the list)
                pressTimestamps[keyName] = undefined;
            }
        }
    }

    var motorVelocity = {x:0.0, y:0.0, z:0.0};

    // figure out if we're pushing or braking
    var accumulatorLength = Vec3.length(motorAccumulator);
    var isPushing = false;
    if (accumulatorLength == 0.0) {
        if (!isBraking) {
            isBraking = true;
        }
        isPushing = false;
    } else {
        isPushing = true;
        motorVelocity = Vec3.multiply(AVATAR_SPEED / accumulatorLength, motorAccumulator);
    }

    // compute the timescale
    var motorTimescale = MAX_MOTOR_TIMESCALE;
    if (isBraking) {
        var speed = Vec3.length(MyAvatar.getVelocity());
        if (speed < MIN_BRAKING_SPEED) {
            // we're going slow enough to turn off braking
            // --> we'll drift to a halt, but not so stiffly that we can't be bumped
            isBraking = false;
            motorTimescale = MAX_MOTOR_TIMESCALE;
        } else {
            // we're still braking
            motorTimescale = BRAKING_MOTOR_TIMESCALE;
        }
    } else if (isPushing) {
        motorTimescale = PUSHING_MOTOR_TIMESCALE;
    }

    // apply the motor parameters
    MyAvatar.motorVelocity = motorVelocity;
    MyAvatar.motorTimescale = motorTimescale;
}

function scriptEnding() {
    // disable the motor
    MyAvatar.motorVelocity = {x:0.0, y:0.0, z:0.0}
    MyAvatar.motorTimescale = VERY_LONG_TIME;
}


// init stuff
MyAvatar.motorReferenceFrame = "camera"; // "camera" is default, other options are "avatar" and "world"
MyAvatar.motorTimescale = VERY_LONG_TIME;

// connect callbacks
Controller.keyPressEvent.connect(keyPressEvent);
Controller.keyReleaseEvent.connect(keyReleaseEvent);
Script.update.connect(updateMotor);
Script.scriptEnding.connect(scriptEnding)
