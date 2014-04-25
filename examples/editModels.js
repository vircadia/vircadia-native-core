//
//  editModels.js
//  examples
//
//  Created by Clément Brisset on 4/24/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//


var LEFT_BUTTON_FWD = 5;
var LEFT_BUTTON_3 = 3;
var RIGHT_BUTTON_FWD = 11;
var RIGHT_BUTTON_3 = 9;
var leftBallAlreadyInHand = false;
var rightBallAlreadyInHand = false;
var leftHandParticle;
var rightHandParticle;
var targetRadius = 0.25;




var LEFT_PALM = 0;
var LEFT_TIP = 1;
var LEFT_TRIGGER = 0;

var RIGHT_PALM = 2;
var RIGHT_TIP = 3;
var RIGHT_TRIGGER = 1;

var previewLineWidth = 4;
var leftLaser = Overlays.addOverlay("line3d", {
                                    position: { x: 0, y: 0, z: 0},
                                    end: { x: 0, y: 0, z: 0},
                                    color: { red: 255, green: 0, blue: 0},
                                    alpha: 1,
                                    visible: false,
                                    lineWidth: previewLineWidth
                                    });
var rightLaser = Overlays.addOverlay("line3d", {
                                     position: { x: 0, y: 0, z: 0},
                                     end: { x: 0, y: 0, z: 0},
                                     color: { red: 255, green: 0, blue: 0},
                                     alpha: 1,
                                     visible: false,
                                     lineWidth: previewLineWidth
                                     });

var particle = Particles.addParticle({ position: { x: 0, y: 0, z:0 },
                                     velocity: { x: 0, y: 0, z: 0},
                                     gravity: { x: 0, y: 0, z: 0},
                                     radius: 0.05,
                                     damping: 0.999,
                                     color: { red: 255, green: 0, blue: 0 },
                                     modelURL: "http://highfidelity-public.s3-us-west-1.amazonaws.com/models/heads/defaultAvatar_head.fst",
                                     })


var wantDebugging = false;
function debugPrint(message) {
    if (wantDebugging) {
        print(message);
    }
}

function getBallHoldPosition(whichSide) {
    var normal;
    var tipPosition;
    if (whichSide == LEFT_PALM) {
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

function checkControllerSide(whichSide) {
    var palmPosition;
    var tipPosition;
    var overlay;
    var triggerValue;
    var BUTTON_3;

    if (whichSide == LEFT_PALM) {
        palmPosition = Controller.getSpatialControlPosition(LEFT_PALM);
        tipPosition = Controller.getSpatialControlPosition(LEFT_TIP);
        overlay = leftLaser;
        triggerValue = Controller.getTriggerValue(LEFT_TRIGGER);
        BUTTON_3 = LEFT_BUTTON_3;
    } else {
        palmPosition = Controller.getSpatialControlPosition(RIGHT_PALM);
        tipPosition = Controller.getSpatialControlPosition(RIGHT_TIP);
        overlay = rightLaser;
        triggerValue = Controller.getTriggerValue(RIGHT_TRIGGER);
        BUTTON_3 = RIGHT_BUTTON_3;
    }
    
    var createButtonPressed = (Controller.isButtonPressed(BUTTON_3));
    if (createButtonPressed) {
        Particles.editParticle(particle, {
                               position: { x: 0, y: 0, z:0 },
                               velocity: { x: 0, y: 0, z: 0},
                               gravity: { x: 0, y: 0, z: 0},
                               radius: 0.05,
                               damping: 0.999,
                               color: { red: 255, green: 0, blue: 0 },
                               modelURL: "http://highfidelity-public.s3-us-west-1.amazonaws.com/models/heads/defaultAvatar_head.fst",
                               });
    }
    
    
    var vector = Vec3.subtract(tipPosition, palmPosition);
    var endPosition = Vec3.sum(palmPosition, Vec3.multiply(vector, 100));
    
    Overlays.editOverlay(overlay, {
                         position: palmPosition,
                         end: endPosition,
                         visible: true
                         });
    
    
    if (triggerValue > 0.9) {
        
    }
    
    return;
    var BUTTON_FWD;
    var BUTTON_3;
    var palmPosition;
    var ballAlreadyInHand;
    var handMessage;
    
    if (whichSide == LEFT_PALM) {
        BUTTON_FWD = LEFT_BUTTON_FWD;
        BUTTON_3 = LEFT_BUTTON_3;
        palmPosition = Controller.getSpatialControlPosition(LEFT_PALM);
        ballAlreadyInHand = leftBallAlreadyInHand;
        handMessage = "LEFT";
    } else {
        BUTTON_FWD = RIGHT_BUTTON_FWD;
        BUTTON_3 = RIGHT_BUTTON_3;
        palmPosition = Controller.getSpatialControlPosition(RIGHT_PALM);
        ballAlreadyInHand = rightBallAlreadyInHand;
        handMessage = "RIGHT";
    }
    
    var grabButtonPressed = (Controller.isButtonPressed(BUTTON_FWD) || Controller.isButtonPressed(BUTTON_3));

    // If I don't currently have a ball in my hand, then try to catch closest one
    if (!ballAlreadyInHand && grabButtonPressed) {
        var closestParticle = Particles.findClosestParticle(palmPosition, targetRadius);

        if (closestParticle.isKnownID) {

            debugPrint(handMessage + " HAND- CAUGHT SOMETHING!!");

            if (whichSide == LEFT_PALM) {
                leftBallAlreadyInHand = true;
                leftHandParticle = closestParticle;
            } else {
                rightBallAlreadyInHand = true;
                rightHandParticle = closestParticle;
            }
            var ballPosition = getBallHoldPosition(whichSide);
            var properties = { position: { x: ballPosition.x, 
                                           y: ballPosition.y, 
                                           z: ballPosition.z }, 
                                velocity : { x: 0, y: 0, z: 0}, inHand: true };
            Particles.editParticle(closestParticle, properties);

            
            return; // exit early
        }
    }

    // change ball color logic...
    //
    //if (wasButtonJustPressed()) {
    //    rotateColor();
    //}

    //  If '3' is pressed, and not holding a ball, make a new one
    if (Controller.isButtonPressed(BUTTON_3) && !ballAlreadyInHand) {
        var ballPosition = getBallHoldPosition(whichSide);
        var properties = { position: ballPosition,
                velocity: { x: 0, y: 0, z: 0}, 
                gravity: { x: 0, y: 0, z: 0}, 
                inHand: true,
                radius: 0.05,
                damping: 0.999,
                color: { red: 255, green: 0, blue: 0 },
                modelURL: "http://highfidelity-public.s3-us-west-1.amazonaws.com/models/heads/defaultAvatar_head.fst",
            };

        newParticle = Particles.addParticle(properties);
        if (whichSide == LEFT_PALM) {
            leftBallAlreadyInHand = true;
            leftHandParticle = newParticle;
        } else {
            rightBallAlreadyInHand = true;
            rightHandParticle = newParticle;
        }
        
        return; // exit early
    }

    if (ballAlreadyInHand) {
        if (whichSide == LEFT_PALM) {
            handParticle = leftHandParticle;
            whichTip = LEFT_TIP;
        } else {
            handParticle = rightHandParticle;
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
            Particles.editParticle(handParticle, properties);
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
                    gravity: { x: 0, y: -2, z: 0}, 
                };

            Particles.editParticle(handParticle, properties);

            if (whichSide == LEFT_PALM) {
                leftBallAlreadyInHand = false;
                leftHandParticle = false;
            } else {
                rightBallAlreadyInHand = false;
                rightHandParticle = false;
            }
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

function scriptEnding() {
    Overlays.deleteOverlay(leftLaser);
    Overlays.deleteOverlay(rightLaser);
}
Script.scriptEnding.connect(scriptEnding);

// register the call back so it fires before each data send
Script.update.connect(checkController);



