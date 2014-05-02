//
//  placeModelsWithHands.js
//  examples
//
//  Created by Brad Hefta-Gaub on 1/20/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
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

var leftModelAlreadyInHand = false;
var rightModelAlreadyInHand = false;
var leftHandModel;
var rightHandModel;

//var throwSound = new Sound("https://dl.dropboxusercontent.com/u/1864924/hifi-sounds/throw.raw");
//var catchSound = new Sound("https://dl.dropboxusercontent.com/u/1864924/hifi-sounds/catch.raw");
var targetRadius = 0.25;


var modelURLs = [
    "http://highfidelity-public.s3-us-west-1.amazonaws.com/meshes/Feisar_Ship.FBX",
    "http://highfidelity-public.s3-us-west-1.amazonaws.com/meshes/birarda/birarda_head.fbx",
    "http://highfidelity-public.s3-us-west-1.amazonaws.com/meshes/pug.fbx",
    "http://highfidelity-public.s3-us-west-1.amazonaws.com/meshes/newInvader16x16-large-purple.svo",
    "http://highfidelity-public.s3-us-west-1.amazonaws.com/meshes/minotaur/mino_full.fbx",
    "http://highfidelity-public.s3-us-west-1.amazonaws.com/meshes/Combat_tank_V01.FBX",
];

var currentModelURL = 0;

var wantDebugging = false;
function debugPrint(message) {
    if (wantDebugging) {
        print(message);
    }
}

function getModelHoldPosition(whichSide) { 
    var normal;
    var tipPosition;
    if (whichSide == LEFT_PALM) {
        normal = Controller.getSpatialControlNormal(LEFT_PALM);
        tipPosition = Controller.getSpatialControlPosition(LEFT_TIP);
    } else {
        normal = Controller.getSpatialControlNormal(RIGHT_PALM);
        tipPosition = Controller.getSpatialControlPosition(RIGHT_TIP);
    }
    
    var MODEL_FORWARD_OFFSET = 0.08; // put the model a bit forward of fingers
    position = { x: MODEL_FORWARD_OFFSET * normal.x,
                 y: MODEL_FORWARD_OFFSET * normal.y, 
                 z: MODEL_FORWARD_OFFSET * normal.z }; 

    position.x += tipPosition.x;
    position.y += tipPosition.y;
    position.z += tipPosition.z;
    
    return position;
}

function checkControllerSide(whichSide) {

    var BUTTON_FWD;
    var BUTTON_3;
    var palmPosition;
    var modelAlreadyInHand;
    var handMessage;
    
    if (whichSide == LEFT_PALM) {
        BUTTON_FWD = LEFT_BUTTON_FWD;
        BUTTON_3 = LEFT_BUTTON_3;
        palmPosition = Controller.getSpatialControlPosition(LEFT_PALM);
        modelAlreadyInHand = leftModelAlreadyInHand;
        handMessage = "LEFT";
    } else {
        BUTTON_FWD = RIGHT_BUTTON_FWD;
        BUTTON_3 = RIGHT_BUTTON_3;
        palmPosition = Controller.getSpatialControlPosition(RIGHT_PALM);
        modelAlreadyInHand = rightModelAlreadyInHand;
        handMessage = "RIGHT";
    }

//print("checkControllerSide..." + handMessage);
    
    var grabButtonPressed = (Controller.isButtonPressed(BUTTON_FWD) || Controller.isButtonPressed(BUTTON_3));

    // If I don't currently have a model in my hand, then try to grab closest one
    if (!modelAlreadyInHand && grabButtonPressed) {
        var closestModel = Models.findClosestModel(palmPosition, targetRadius);

        if (closestModel.isKnownID) {

            debugPrint(handMessage + " HAND- CAUGHT SOMETHING!!");

            if (whichSide == LEFT_PALM) {
                leftModelAlreadyInHand = true;
                leftHandModel = closestModel;
            } else {
                rightModelAlreadyInHand = true;
                rightHandModel = closestModel;
            }
            var modelPosition = getModelHoldPosition(whichSide);
            var properties = { position: { x: modelPosition.x, 
                                           y: modelPosition.y, 
                                           z: modelPosition.z }, 
                              };
            Models.editModel(closestModel, properties);

            /*            
    		var options = new AudioInjectionOptions();
			options.position = modelPosition;
			options.volume = 1.0;
			Audio.playSound(catchSound, options);
			*/
            
            return; // exit early
        }
    }

    //  If '3' is pressed, and not holding a model, make a new one
    if (Controller.isButtonPressed(BUTTON_3) && !modelAlreadyInHand) {
        var modelPosition = getModelHoldPosition(whichSide);
        var properties = { position: { x: modelPosition.x, 
                                       y: modelPosition.y, 
                                       z: modelPosition.z }, 
                radius: 0.25,
                modelURL: modelURLs[currentModelURL]
            };

        newModel = Models.addModel(properties);
        if (whichSide == LEFT_PALM) {
            leftModelAlreadyInHand = true;
            leftHandModel = newModel;
        } else {
            rightModelAlreadyInHand = true;
            rightHandModel = newModel;
        }

        // Play a new model sound
        /*
        var options = new AudioInjectionOptions();
        options.position = modelPosition;
        options.volume = 1.0;
        Audio.playSound(catchSound, options);
        */
        
        return; // exit early
    }

    if (modelAlreadyInHand) {
        if (whichSide == LEFT_PALM) {
            handModel = leftHandModel;
            whichTip = LEFT_TIP;
        } else {
            handModel = rightHandModel;
            whichTip = RIGHT_TIP;
        }

        //  If holding the model keep it in the palm
        if (grabButtonPressed) {
            debugPrint(">>>>> " + handMessage + "-MODEL IN HAND, grabbing, hold and move");
            var modelPosition = getModelHoldPosition(whichSide);
            var properties = { position: { x: modelPosition.x, 
                                           y: modelPosition.y, 
                                           z: modelPosition.z }, 
                };
            Models.editModel(handModel, properties);
        } else {
            debugPrint(">>>>> " + handMessage + "-MODEL IN HAND, not grabbing, RELEASE!!!");
            if (whichSide == LEFT_PALM) {
                leftModelAlreadyInHand = false;
                leftHandModel = false;
            } else {
                rightModelAlreadyInHand = false;
                rightHandModel = false;
            }

            /*
    		var options = new AudioInjectionOptions();
			options.position = modelPosition;
			options.volume = 1.0;
			Audio.playSound(throwSound, options);
			*/
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
