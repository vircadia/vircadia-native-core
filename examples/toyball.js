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

var leftBallAlreadyInHand = false;
var rightBallAlreadyInHand = false;
var leftHandParticle;
var rightHandParticle;

function checkController() {
    var numberOfButtons = Controller.getNumberOfButtons();
    var numberOfTriggers = Controller.getNumberOfTriggers();
    var numberOfSpatialControls = Controller.getNumberOfSpatialControls();
    var controllersPerTrigger = numberOfSpatialControls / numberOfTriggers;

    // this is expected for hydras
    if (!(numberOfButtons==12 && numberOfTriggers == 2 && controllersPerTrigger == 2)) {
        print("no hydra connected?");
        return; // bail if no hydra
    }

    // maybe we should make these constants...
    var LEFT_PALM = 0;
    var LEFT_TIP = 1;
    var LEFT_BUTTON_FWD = 5;
    var LEFT_BUTTON_3 = 3;

    var RIGHT_PALM = 2;
    var RIGHT_TIP = 3;
    var RIGHT_BUTTON_FWD = 11;
    var RIGHT_BUTTON_3 = 9;

    var leftGrabButtonPressed = (Controller.isButtonPressed(LEFT_BUTTON_FWD) || Controller.isButtonPressed(LEFT_BUTTON_3));
    var rightGrabButtonPressed = (Controller.isButtonPressed(RIGHT_BUTTON_FWD) || Controller.isButtonPressed(RIGHT_BUTTON_3));

    var leftPalmPosition = Controller.getSpatialControlPosition(LEFT_PALM);
    //print("leftPalmPosition: " + leftPalmPosition.x + ", "+ leftPalmPosition.y + ", "+ leftPalmPosition.z);

    var rightPalmPosition = Controller.getSpatialControlPosition(RIGHT_PALM);
    //print("rightPalmPosition: " + rightPalmPosition.x + ", "+ rightPalmPosition.y + ", "+ rightPalmPosition.z);

    var targetRadius = 3.0;

    // If I don't currently have a ball in my left hand, then try to catch closest one
    if (!leftBallAlreadyInHand && leftGrabButtonPressed) {
        var closestParticle = Particles.findClosestParticle(leftPalmPosition, targetRadius);

        if (closestParticle.isKnownID) {

print("LEFT HAND- CAUGHT SOMETHING!!");

            leftBallAlreadyInHand = true;
            leftHandParticle = closestParticle;
            var properties = { position: { x: leftPalmPosition.x / TREE_SCALE, 
                                           y: leftPalmPosition.y / TREE_SCALE, 
                                           z: leftPalmPosition.z / TREE_SCALE }, 
                                velocity : { x: 0, y: 0, z: 0}, inHand: true };
            Particles.editParticle(leftHandParticle, properties);
            
            /**
            AudioInjectorOptions injectorOptions;
            injectorOptions.setPosition(newPosition);
            injectorOptions.setLoopbackAudioInterface(app->getAudio());
    
            AudioScriptingInterface::playSound(&_catchSound, &injectorOptions);
            **/
            
            return; // exit early
        }
    }

    // If I don't currently have a ball in my right hand, then try to catch closest one
    if (!rightBallAlreadyInHand && rightGrabButtonPressed) {
        var closestParticle = Particles.findClosestParticle(rightPalmPosition, targetRadius);

        if (closestParticle.isKnownID) {

print("RIGHT HAND- CAUGHT SOMETHING!!");

            rightBallAlreadyInHand = true;
            rightHandParticle = closestParticle;
            var properties = { position: { x: rightPalmPosition.x / TREE_SCALE, 
                                           y: rightPalmPosition.y / TREE_SCALE, 
                                           z: rightPalmPosition.z / TREE_SCALE }, 
                                velocity : { x: 0, y: 0, z: 0}, inHand: true };
            Particles.editParticle(rightHandParticle, properties);
            
            /**
            AudioInjectorOptions injectorOptions;
            injectorOptions.setPosition(newPosition);
            injectorOptions.setLoopbackAudioInterface(app->getAudio());
    
            AudioScriptingInterface::playSound(&_catchSound, &injectorOptions);
            **/
            
            return; // exit early
        }
    }

    // change ball color logic...
    /***
    if (currentControllerButtons != _lastControllerButtons && (currentControllerButtons & BUTTON_0)) {
        _whichBallColor[handID]++;
        if (_whichBallColor[handID] >= sizeof(TOY_BALL_ON_SERVER_COLOR)/sizeof(TOY_BALL_ON_SERVER_COLOR[0])) {
            _whichBallColor[handID] = 0;
        }
    }
    ***/

    //  LEFT HAND -- If '3' is pressed, and not holding a ball, make a new one
    if (Controller.isButtonPressed(LEFT_BUTTON_3) && !leftBallAlreadyInHand) {
        leftBallAlreadyInHand = true;
        var properties = { 
                position: { x: leftPalmPosition.x / TREE_SCALE, 
                            y: leftPalmPosition.y / TREE_SCALE, 
                            z: leftPalmPosition.z / TREE_SCALE } ,
                velocity: { x: 0, y: 0, z: 0}, 
                gravity: { x: 0, y: 0, z: 0}, 
                inHand: true,
                radius: 0.05 / TREE_SCALE,
                color: { red: 255, green: 0, blue: 0 },
                lifetime: 30
            };

print("calling addParticle()");
        leftHandParticle = Particles.addParticle(properties);
print("leftHandParticle=" + leftHandParticle.creatorTokenID);

        // Play a new ball sound
        //app->getAudio()->startDrumSound(1.0f, 2000, 0.5f, 0.02f);
        
        return; // exit early
    }

    // RIGHT HAND --  If '3' is pressed, and not holding a ball, make a new one
    if (Controller.isButtonPressed(RIGHT_BUTTON_3) && !rightBallAlreadyInHand) {
        rightBallAlreadyInHand = true;
        var properties = { 
                position: { x: rightPalmPosition.x / TREE_SCALE, 
                            y: rightPalmPosition.y / TREE_SCALE, 
                            z: rightPalmPosition.z / TREE_SCALE } ,
                velocity: { x: 0, y: 0, z: 0}, 
                gravity: { x: 0, y: 0, z: 0}, 
                inHand: true,
                radius: 0.05 / TREE_SCALE,
                color: { red: 255, green: 255, blue: 0 },
                lifetime: 30
            };

print("calling addParticle()");
        rightHandParticle = Particles.addParticle(properties);
print("rightHandParticle=" + rightHandParticle.creatorTokenID);

        // Play a new ball sound
        //app->getAudio()->startDrumSound(1.0f, 2000, 0.5f, 0.02f);

        return; // exit early
    }


    if (leftBallAlreadyInHand) {
        //  If holding the ball keep it in the palm
        if (leftGrabButtonPressed) {
            var properties = { 
                    position: { x: leftPalmPosition.x / TREE_SCALE, 
                                y: leftPalmPosition.y / TREE_SCALE, 
                                z: leftPalmPosition.z / TREE_SCALE } ,
                };

print("Particles.editParticle()... staying in hand leftHandParticle=" + leftHandParticle.creatorTokenID);
            Particles.editParticle(leftHandParticle, properties);
print("leftPalmPosition: " + leftPalmPosition.x + ", "+ leftPalmPosition.y + ", "+ leftPalmPosition.z);
print("leftPalmPosition: " + leftPalmPosition.x  / TREE_SCALE + ", "+ leftPalmPosition.y / TREE_SCALE + ", "+ leftPalmPosition.z  / TREE_SCALE);
        } else {
print(">>>>> LEFT-BALL IN HAND, but NOT GRABBING!!!");
        
            //  If toy ball just released, add velocity to it!
            var leftTipVelocity = Controller.getSpatialControlVelocity(LEFT_TIP);

            var properties = { 
                    velocity: { x: leftTipVelocity.x / TREE_SCALE, 
                                y: leftTipVelocity.y / TREE_SCALE, 
                                z: leftTipVelocity.z / TREE_SCALE } ,
                    inHand: false,
                    gravity: { x: 0, y: -2 / TREE_SCALE, z: 0}, 
                };

print("leftTipVelocity: " + leftTipVelocity.x + ", "+ leftTipVelocity.y + ", "+ leftTipVelocity.z);
print("leftTipVelocity: " + leftTipVelocity.x  / TREE_SCALE + ", "+ leftTipVelocity.y / TREE_SCALE + ", "+ leftTipVelocity.z  / TREE_SCALE);

print("Particles.editParticle()... releasing leftHandParticle=" + leftHandParticle.creatorTokenID);
            Particles.editParticle(leftHandParticle, properties);
            leftBallAlreadyInHand = false;
            leftHandParticle = false;

            /**
            AudioInjectorOptions injectorOptions;
            injectorOptions.setPosition(ballPosition);
            injectorOptions.setLoopbackAudioInterface(app->getAudio());
            
            AudioScriptingInterface::playSound(&_throwSound, &injectorOptions);
            **/
        }
    }

    if (rightBallAlreadyInHand) {
        //  If holding the ball keep it in the palm
        if (rightGrabButtonPressed) {
            var properties = { 
                    position: { x: rightPalmPosition.x / TREE_SCALE, 
                                y: rightPalmPosition.y / TREE_SCALE, 
                                z: rightPalmPosition.z / TREE_SCALE } ,
                };

print("Particles.editParticle()... staying in hand rightHandParticle=" + rightHandParticle.creatorTokenID);
            Particles.editParticle(rightHandParticle, properties);
print("rightPalmPosition: " + rightPalmPosition.x + ", "+ rightPalmPosition.y + ", "+ rightPalmPosition.z);
print("rightPalmPosition: " + rightPalmPosition.x  / TREE_SCALE + ", "+ rightPalmPosition.y / TREE_SCALE + ", "+ rightPalmPosition.z  / TREE_SCALE);

        } else {
print(">>>>> RIGHT-BALL IN HAND, but NOT GRABBING!!!");

            //  If toy ball just released, add velocity to it!
            var rightTipVelocity = Controller.getSpatialControlVelocity(LEFT_TIP);

            var properties = { 
                    velocity: { x: rightTipVelocity.x / TREE_SCALE, 
                                y: rightTipVelocity.y / TREE_SCALE, 
                                z: rightTipVelocity.z / TREE_SCALE } ,
                    inHand: false,
                    gravity: { x: 0, y: -2 / TREE_SCALE, z: 0}, 
                };

print("Particles.editParticle()... releasing rightHandParticle=" + rightHandParticle.creatorTokenID);
            Particles.editParticle(rightHandParticle, properties);
            rightBallAlreadyInHand = false;
            rightHandParticle = false;

            /**
            AudioInjectorOptions injectorOptions;
            injectorOptions.setPosition(ballPosition);
            injectorOptions.setLoopbackAudioInterface(app->getAudio());
            
            AudioScriptingInterface::playSound(&_throwSound, &injectorOptions);
            **/
        }
    }
}


// register the call back so it fires before each data send
Agent.willSendVisualDataCallback.connect(checkController);
