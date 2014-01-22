//
//  toyball.js
//  hifi
//
//  Created by Brad Hefta-Gaub on 12/31/13.
//  Copyright (c) 2013 HighFidelity, Inc. All rights reserved.
//
//  This is an example script that turns the hydra controllers into a toy ball catch and throw game.
//  It reads the controller, watches for button presses and trigger pulls, and launches particles.
//
//  The particles it creates have a script that when they collide with Voxels, the
//  particle will change it's color to match the voxel it hits.
//
//

// maybe we should make these constants...
var LEFT_PALM = 0;
var LEFT_TIP = 1;
var LEFT_BUTTON_FWD = 5;
var LEFT_BUTTON_3 = 3;

var RIGHT_PALM = 2;
var RIGHT_TIP = 3;
var RIGHT_BUTTON_FWD = 11;
var RIGHT_BUTTON_3 = 9;

var leftBallAlreadyInHand = false;
var rightBallAlreadyInHand = false;
var leftHandParticle;
var rightHandParticle;

var throwSound = new Sound("https://dl.dropboxusercontent.com/u/1864924/hifi-sounds/throw.raw");
var catchSound = new Sound("https://dl.dropboxusercontent.com/u/1864924/hifi-sounds/catch.raw");

function getBallHoldPosition(leftHand) { 
    var normal;
    var tipPosition;
    if (leftHand) {
        normal = Controller.getSpatialControlNormal(LEFT_PALM);
        tipPosition = Controller.getSpatialControlPosition(LEFT_TIP);
    } else {
        normal = Controller.getSpatialControlNormal(RIGHT_PALM);
        tipPosition = Controller.getSpatialControlPosition(RIGHT_TIP);
    }
    
    var BALL_FORWARD_OFFSET = 0.08; // put the ball a bit forward of fingers
    position = { x: BALL_FORWARD_OFFSET * normal.x,
                 y: BALL_FORWARD_OFFSET * normal.y, 
                 z: BALL_FORWARD_OFFSET * normal.z }; 

    position.x += tipPosition.x;
    position.y += tipPosition.y;
    position.z += tipPosition.z;
    
    return position;
}

var wantDebugging = false;
function debugPrint(message) {
    if (wantDebugging) {
        print(message);
    }
}


function checkController() {
    var numberOfButtons = Controller.getNumberOfButtons();
    var numberOfTriggers = Controller.getNumberOfTriggers();
    var numberOfSpatialControls = Controller.getNumberOfSpatialControls();
    var controllersPerTrigger = numberOfSpatialControls / numberOfTriggers;

    // this is expected for hydras
    if (!(numberOfButtons==12 && numberOfTriggers == 2 && controllersPerTrigger == 2)) {
        debugPrint("no hydra connected?");
        return; // bail if no hydra
    }

    var leftGrabButtonPressed = (Controller.isButtonPressed(LEFT_BUTTON_FWD) || Controller.isButtonPressed(LEFT_BUTTON_3));
    var rightGrabButtonPressed = (Controller.isButtonPressed(RIGHT_BUTTON_FWD) || Controller.isButtonPressed(RIGHT_BUTTON_3));

    var leftPalmPosition = Controller.getSpatialControlPosition(LEFT_PALM);
    var rightPalmPosition = Controller.getSpatialControlPosition(RIGHT_PALM);

    var targetRadius = 3.0;
    var exitEarly = false;

    // If I don't currently have a ball in my left hand, then try to catch closest one
    if (!leftBallAlreadyInHand && leftGrabButtonPressed) {
        var closestParticle = Particles.findClosestParticle(leftPalmPosition, targetRadius);

        if (closestParticle.isKnownID) {

            debugPrint("LEFT HAND- CAUGHT SOMETHING!!");

            leftBallAlreadyInHand = true;
            leftHandParticle = closestParticle;
            var ballPosition = getBallHoldPosition(true);
            var properties = { position: { x: ballPosition.x / TREE_SCALE, 
                                           y: ballPosition.y / TREE_SCALE, 
                                           z: ballPosition.z / TREE_SCALE }, 
                                velocity : { x: 0, y: 0, z: 0}, inHand: true };
            Particles.editParticle(leftHandParticle, properties);
            
    		var options = new AudioInjectionOptions(); 
			options.position = ballPosition;
			options.volume = 1.0;
			Audio.playSound(catchSound, options);
            
            exitEarly = true;
        }
    }

    // If I don't currently have a ball in my right hand, then try to catch closest one
    if (!rightBallAlreadyInHand && rightGrabButtonPressed) {
        var closestParticle = Particles.findClosestParticle(rightPalmPosition, targetRadius);

        if (closestParticle.isKnownID) {

            debugPrint("RIGHT HAND- CAUGHT SOMETHING!!");

            rightBallAlreadyInHand = true;
            rightHandParticle = closestParticle;

            var ballPosition = getBallHoldPosition(false); // false == right hand
            
            var properties = { position: { x: ballPosition.x / TREE_SCALE, 
                                           y: ballPosition.y / TREE_SCALE, 
                                           z: ballPosition.z / TREE_SCALE }, 
                                velocity : { x: 0, y: 0, z: 0}, inHand: true };
            Particles.editParticle(rightHandParticle, properties);
            
    		var options = new AudioInjectionOptions(); 
			options.position = ballPosition;
			options.volume = 1.0;
			Audio.playSound(catchSound, options);
            
            exitEarly = true;
        }
    }

    // If we did one of the actions above, then exit now
    if (exitEarly) {
        debugPrint("exiting early - caught something");
        return;
    }

    // change ball color logic...
    //
    //if (wasButtonJustPressed()) {
    //    rotateColor();
    //}

    //  LEFT HAND -- If '3' is pressed, and not holding a ball, make a new one
    if (Controller.isButtonPressed(LEFT_BUTTON_3) && !leftBallAlreadyInHand) {
        leftBallAlreadyInHand = true;
        var ballPosition = getBallHoldPosition(true); // true == left hand
        var properties = { position: { x: ballPosition.x / TREE_SCALE, 
                                       y: ballPosition.y / TREE_SCALE, 
                                       z: ballPosition.z / TREE_SCALE }, 
                velocity: { x: 0, y: 0, z: 0}, 
                gravity: { x: 0, y: 0, z: 0}, 
                inHand: true,
                radius: 0.05 / TREE_SCALE,
                color: { red: 255, green: 0, blue: 0 },
                lifetime: 30
            };

        leftHandParticle = Particles.addParticle(properties);

        // Play a new ball sound
        var options = new AudioInjectionOptions(); 
        options.position = ballPosition;
        options.volume = 1.0;
        Audio.playSound(catchSound, options);
        
        exitEarly = true;
    }

    // RIGHT HAND --  If '3' is pressed, and not holding a ball, make a new one
    if (Controller.isButtonPressed(RIGHT_BUTTON_3) && !rightBallAlreadyInHand) {
        rightBallAlreadyInHand = true;
        var ballPosition = getBallHoldPosition(false); // false == right hand
        var properties = { position: { x: ballPosition.x / TREE_SCALE, 
                                       y: ballPosition.y / TREE_SCALE, 
                                       z: ballPosition.z / TREE_SCALE }, 
                velocity: { x: 0, y: 0, z: 0}, 
                gravity: { x: 0, y: 0, z: 0}, 
                inHand: true,
                radius: 0.05 / TREE_SCALE,
                color: { red: 255, green: 255, blue: 0 },
                lifetime: 30
            };

        rightHandParticle = Particles.addParticle(properties);

        // Play a new ball sound
        var options = new AudioInjectionOptions(); 
        options.position = ballPosition;
        options.volume = 1.0;
        Audio.playSound(catchSound, options);

        exitEarly = true;
    }
    
    // If we did one of the actions above, then exit now
    if (exitEarly) {
        debugPrint("exiting early - ball created");
        return;
    }

    if (leftBallAlreadyInHand) {
        //  If holding the ball keep it in the palm
        if (leftGrabButtonPressed) {
            debugPrint(">>>>> LEFT-BALL IN HAND, grabbing, hold and move");
            var ballPosition = getBallHoldPosition(true); // true == left hand
            var properties = { position: { x: ballPosition.x / TREE_SCALE, 
                                           y: ballPosition.y / TREE_SCALE, 
                                           z: ballPosition.z / TREE_SCALE }, 
                };
            Particles.editParticle(leftHandParticle, properties);
        } else {
            debugPrint(">>>>> LEFT-BALL IN HAND, not grabbing, THROW!!!");
            //  If toy ball just released, add velocity to it!
            var tipVelocity = Controller.getSpatialControlVelocity(LEFT_TIP);
            var THROWN_VELOCITY_SCALING = 1.5;
            var properties = { 
                    velocity: { x: (tipVelocity.x * THROWN_VELOCITY_SCALING) / TREE_SCALE, 
                                y: (tipVelocity.y * THROWN_VELOCITY_SCALING) / TREE_SCALE, 
                                z: (tipVelocity.z * THROWN_VELOCITY_SCALING) / TREE_SCALE } ,
                    inHand: false,
                    gravity: { x: 0, y: -2 / TREE_SCALE, z: 0}, 
                };

            Particles.editParticle(leftHandParticle, properties);
            leftBallAlreadyInHand = false;
            leftHandParticle = false;

    		var options = new AudioInjectionOptions(); 
			options.position = ballPosition;
			options.volume = 1.0;
			Audio.playSound(throwSound, options);
        }
    }

    if (rightBallAlreadyInHand) {
        //  If holding the ball keep it in the palm
        if (rightGrabButtonPressed) {
            debugPrint(">>>>> RIGHT-BALL IN HAND, grabbing, hold and move");
            var ballPosition = getBallHoldPosition(false); // false == right hand
            var properties = { position: { x: ballPosition.x / TREE_SCALE, 
                                           y: ballPosition.y / TREE_SCALE, 
                                           z: ballPosition.z / TREE_SCALE }, 
                             };
            Particles.editParticle(rightHandParticle, properties);
        } else {
            debugPrint(">>>>> RIGHT-BALL IN HAND, not grabbing, THROW!!!");

            //  If toy ball just released, add velocity to it!
            var tipVelocity = Controller.getSpatialControlVelocity(RIGHT_TIP);
            var THROWN_VELOCITY_SCALING = 1.5;
            var properties = { 
                    velocity: { x: (tipVelocity.x * THROWN_VELOCITY_SCALING) / TREE_SCALE, 
                                y: (tipVelocity.y * THROWN_VELOCITY_SCALING) / TREE_SCALE, 
                                z: (tipVelocity.z * THROWN_VELOCITY_SCALING) / TREE_SCALE } ,
                    inHand: false,
                    gravity: { x: 0, y: -2 / TREE_SCALE, z: 0}, 
                };

            Particles.editParticle(rightHandParticle, properties);
            rightBallAlreadyInHand = false;
            rightHandParticle = false;

    		var options = new AudioInjectionOptions(); 
			options.position = ballPosition;
			options.volume = 1.0;
			Audio.playSound(throwSound, options);
        }
    }
}


// register the call back so it fires before each data send
Agent.willSendVisualDataCallback.connect(checkController);
