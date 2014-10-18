//
//  toyball.js
//  examples
//
//  Created by Brad Hefta-Gaub on 1/20/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  This is an example script that turns the hydra controllers into a toy ball catch and throw game.
//  It reads the controller, watches for button presses and trigger pulls, and launches entities.
//
//  The entities it creates have a script that when they collide with Voxels, the
//  entity will change it's color to match the voxel it hits.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

Script.include("libraries/globals.js");

// maybe we should make these constants...
var LEFT_PALM = 0;
var LEFT_TIP = 1;
var LEFT_BUTTON_FWD = 5;
var LEFT_BUTTON_3 = 3;

var RIGHT_PALM = 2;
var RIGHT_TIP = 3;
var RIGHT_BUTTON_FWD = 11;
var RIGHT_BUTTON_3 = 9;

var BALL_RADIUS = 0.08;
var GRAVITY_STRENGTH = 0.5;

var HELD_COLOR = { red: 240, green: 0, blue: 0 };
var THROWN_COLOR = { red: 128, green: 0, blue: 0 };

var leftBallAlreadyInHand = false;
var rightBallAlreadyInHand = false;
var leftHandEntity;
var rightHandEntity;

var newSound = new Sound("https://dl.dropboxusercontent.com/u/1864924/hifi-sounds/throw.raw");
var catchSound = new Sound("https://dl.dropboxusercontent.com/u/1864924/hifi-sounds/catch.raw");
var throwSound = new Sound(HIFI_PUBLIC_BUCKET + "sounds/Switches%20and%20sliders/slider%20-%20whoosh1.raw");
var targetRadius = 1.0;


var wantDebugging = false;
function debugPrint(message) {
    if (wantDebugging) {
        print(message);
    }
}

function getBallHoldPosition(whichSide) { 
    if (whichSide == LEFT_PALM) {
        position = MyAvatar.getLeftPalmPosition();
    } else {
        position = MyAvatar.getRightPalmPosition();
    }
    
    return position;
}

function checkControllerSide(whichSide) {
    var BUTTON_FWD;
    var BUTTON_3;
    var TRIGGER;
    var palmPosition;
    var ballAlreadyInHand;
    var handMessage;
    
    if (whichSide == LEFT_PALM) {
        BUTTON_FWD = LEFT_BUTTON_FWD;
        BUTTON_3 = LEFT_BUTTON_3;
        TRIGGER = 0;
        palmPosition = Controller.getSpatialControlPosition(LEFT_PALM);
        ballAlreadyInHand = leftBallAlreadyInHand;
        handMessage = "LEFT";
    } else {
        BUTTON_FWD = RIGHT_BUTTON_FWD;
        BUTTON_3 = RIGHT_BUTTON_3;
        TRIGGER = 1;
        palmPosition = Controller.getSpatialControlPosition(RIGHT_PALM);
        ballAlreadyInHand = rightBallAlreadyInHand;
        handMessage = "RIGHT";
    }

    var grabButtonPressed = (Controller.isButtonPressed(BUTTON_FWD) || Controller.isButtonPressed(BUTTON_3) || (Controller.getTriggerValue(TRIGGER) > 0.5));

    // If I don't currently have a ball in my hand, then try to catch closest one
    if (!ballAlreadyInHand && grabButtonPressed) {
        var closestEntity = Entities.findClosestEntity(palmPosition, targetRadius);

        if (closestEntity.isKnownID) {

            debugPrint(handMessage + " HAND- CAUGHT SOMETHING!!");

            if (whichSide == LEFT_PALM) {
                leftBallAlreadyInHand = true;
                leftHandEntity = closestEntity;
            } else {
                rightBallAlreadyInHand = true;
                rightHandEntity = closestEntity;
            }
            var ballPosition = getBallHoldPosition(whichSide);
            var properties = { position: { x: ballPosition.x, 
                                           y: ballPosition.y, 
                                           z: ballPosition.z },
                                           color: HELD_COLOR, 
                                velocity : { x: 0, y: 0, z: 0}, 
                                lifetime : 600,
                                inHand: true };
            Entities.editEntity(closestEntity, properties);
            
    		var options = new AudioInjectionOptions();
			options.position = ballPosition;
			options.volume = 1.0;
			Audio.playSound(catchSound, options);
            
            return; // exit early
        }
    }

    // change ball color logic...
    //
    //if (wasButtonJustPressed()) {
    //    rotateColor();
    //}

    //  If '3' is pressed, and not holding a ball, make a new one
    if (grabButtonPressed && !ballAlreadyInHand) {
        var ballPosition = getBallHoldPosition(whichSide);
        var properties = { 
                type: "Sphere",
                position: { x: ballPosition.x, 
                            y: ballPosition.y, 
                            z: ballPosition.z }, 
                velocity: { x: 0, y: 0, z: 0}, 
                gravity: { x: 0, y: 0, z: 0}, 
                inHand: true,
                radius: { x: BALL_RADIUS * 2, y: BALL_RADIUS * 2, z: BALL_RADIUS * 2 },
                damping: 0.999,
                color: HELD_COLOR,

                lifetime: 600 // 10 seconds - same as default, not needed but here as an example
            };

        newEntity = Entities.addEntity(properties);
        if (whichSide == LEFT_PALM) {
            leftBallAlreadyInHand = true;
            leftHandEntity = newEntity;
        } else {
            rightBallAlreadyInHand = true;
            rightHandEntity = newEntity;
        }

        // Play a new ball sound
        var options = new AudioInjectionOptions();
        options.position = ballPosition;
        options.volume = 1.0;
        Audio.playSound(newSound, options);
        
        return; // exit early
    }

    if (ballAlreadyInHand) {
        if (whichSide == LEFT_PALM) {
            handEntity = leftHandEntity;
            whichTip = LEFT_TIP;
        } else {
            handEntity = rightHandEntity;
            whichTip = RIGHT_TIP;
        }

        //  If holding the ball keep it in the palm
        if (grabButtonPressed) {
            debugPrint(">>>>> " + handMessage + "-BALL IN HAND, grabbing, hold and move");
            var ballPosition = getBallHoldPosition(whichSide);
            var properties = { position: { x: ballPosition.x, 
                                           y: ballPosition.y, 
                                           z: ballPosition.z }, 
                };
            Entities.editEntity(handEntity, properties);
        } else {
            debugPrint(">>>>> " + handMessage + "-BALL IN HAND, not grabbing, THROW!!!");
            //  If toy ball just released, add velocity to it!
            var tipVelocity = Controller.getSpatialControlVelocity(whichTip);
            var THROWN_VELOCITY_SCALING = 1.5;
            var properties = { 
                    velocity: { x: tipVelocity.x * THROWN_VELOCITY_SCALING, 
                                y: tipVelocity.y * THROWN_VELOCITY_SCALING, 
                                z: tipVelocity.z * THROWN_VELOCITY_SCALING } ,
                    inHand: false,
                    color: THROWN_COLOR,
                    lifetime: 10,
                    gravity: { x: 0, y: -GRAVITY_STRENGTH, z: 0}, 
                };

            Entities.editEntity(handEntity, properties);

            if (whichSide == LEFT_PALM) {
                leftBallAlreadyInHand = false;
                leftHandEntity = false;
            } else {
                rightBallAlreadyInHand = false;
                rightHandEntity = false;
            }

    		var options = new AudioInjectionOptions();
			options.position = ballPosition;
			options.volume = 1.0;
			Audio.playSound(throwSound, options);
        }
    }
}


function checkController(deltaTime) {
    var numberOfButtons = Controller.getNumberOfButtons();
    var numberOfTriggers = Controller.getNumberOfTriggers();
    var numberOfSpatialControls = Controller.getNumberOfSpatialControls();
    var controllersPerTrigger = numberOfSpatialControls / numberOfTriggers;

    // this is expected for hydras
    if (!(numberOfButtons==12 && numberOfTriggers == 2 && controllersPerTrigger == 2)) {
        debugPrint("no hydra connected?");
        return; // bail if no hydra
    }

    checkControllerSide(LEFT_PALM);
    checkControllerSide(RIGHT_PALM);
}


// register the call back so it fires before each data send
Script.update.connect(checkController);
