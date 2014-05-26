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
var leftRecentlyDeleted = false;
var rightRecentlyDeleted = false;
var leftHandModel;
var rightHandModel;

//var throwSound = new Sound("https://dl.dropboxusercontent.com/u/1864924/hifi-sounds/throw.raw");
//var catchSound = new Sound("https://dl.dropboxusercontent.com/u/1864924/hifi-sounds/catch.raw");
var targetRadius = 0.5;

var radiusDefault = 0.25;
var modelRadius = radiusDefault;
var radiusMinimum = 0.05;
var radiusMaximum = 0.5;

var modelURLs = [
    "http://www.fungibleinsight.com/faces/beta.fst",
    "https://s3-us-west-1.amazonaws.com/highfidelity-public/models/attachments/topHat.fst",
    "http://highfidelity-public.s3-us-west-1.amazonaws.com/meshes/Feisar_Ship.FBX",
    "http://highfidelity-public.s3-us-west-1.amazonaws.com/meshes/birarda/birarda_head.fbx",
    "http://highfidelity-public.s3-us-west-1.amazonaws.com/meshes/pug.fbx",
    "http://highfidelity-public.s3-us-west-1.amazonaws.com/meshes/newInvader16x16-large-purple.svo",
    "http://highfidelity-public.s3-us-west-1.amazonaws.com/meshes/minotaur/mino_full.fbx",
    "http://highfidelity-public.s3-us-west-1.amazonaws.com/meshes/Combat_tank_V01.FBX",
    "http://highfidelity-public.s3-us-west-1.amazonaws.com/meshes/orc.fbx",
    "http://highfidelity-public.s3-us-west-1.amazonaws.com/meshes/slimer.fbx",
];

var animationURLs = [
    "http://www.fungibleinsight.com/faces/gangnam_style_2.fbx",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
    "",
];

var currentModelURL = 1;
var numModels = modelURLs.length;


function getNewVoxelPosition() {
    var camera = Camera.getPosition();
    var forwardVector = Quat.getFront(MyAvatar.orientation);
    var newPosition = Vec3.sum(camera, Vec3.multiply(forwardVector, 2.0));
    return newPosition;
}

function keyPressEvent(event) {
    debugPrint("event.text=" + event.text);
    if (event.text == "ESC") {
        if (leftRecentlyDeleted) {
            leftRecentlyDeleted = false;
            leftModelAlreadyInHand = false;
        }
        if (rightRecentlyDeleted) {
            rightRecentlyDeleted = false;
            rightModelAlreadyInHand = false;
        }
    } else if (event.text == "m") {
        var URL = Window.prompt("Model URL", "Enter URL, e.g. http://foo.com/model.fbx");
        var modelPosition = getNewVoxelPosition();
        var properties = { position: { x: modelPosition.x, 
                                       y: modelPosition.y, 
                                       z: modelPosition.z }, 
                radius: modelRadius,
                modelURL: URL
            };
        newModel = Models.addModel(properties);

    } else if (event.text == "DELETE" || event.text == "BACKSPACE") {

        if (leftModelAlreadyInHand) {
            print("want to delete leftHandModel=" + leftHandModel);
            Models.deleteModel(leftHandModel);
            leftHandModel = "";
            //leftModelAlreadyInHand = false;
            leftRecentlyDeleted = true;
        }
        if (rightModelAlreadyInHand) {
            print("want to delete rightHandModel=" + rightHandModel);
            Models.deleteModel(rightHandModel);
            rightHandModel = "";
            //rightModelAlreadyInHand = false;
            rightRecentlyDeleted = true;
        }
    } else {
        var nVal = parseInt(event.text);
        if ((nVal >= 0) && (nVal < numModels)) {
            currentModelURL = nVal;
            print("Model = " + currentModelURL);
        }
    }
}

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
    var palmRotation;
    var modelAlreadyInHand;
    var handMessage;
    
    if (whichSide == LEFT_PALM) {
        BUTTON_FWD = LEFT_BUTTON_FWD;
        BUTTON_3 = LEFT_BUTTON_3;
        palmPosition = Controller.getSpatialControlPosition(LEFT_PALM);
        palmRotation = Controller.getSpatialControlRawRotation(LEFT_PALM);
        modelAlreadyInHand = leftModelAlreadyInHand;
        handMessage = "LEFT";
    } else {
        BUTTON_FWD = RIGHT_BUTTON_FWD;
        BUTTON_3 = RIGHT_BUTTON_3;
        palmPosition = Controller.getSpatialControlPosition(RIGHT_PALM);
        palmRotation = Controller.getSpatialControlRawRotation(RIGHT_PALM);
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
                                radius: modelRadius,
                                modelRotation: palmRotation,
                              };

            debugPrint(">>>>>>>>>>>> EDIT MODEL.... modelRadius=" +modelRadius);

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
                radius: modelRadius,
                modelRotation: palmRotation,
                modelURL: modelURLs[currentModelURL]
            };
            
        if (animationURLs[currentModelURL] !== "") {
            properties.animationURL = animationURLs[currentModelURL];
            properties.animationIsPlaying = true;
        }

        debugPrint("modelRadius=" +modelRadius);

        //newModel = Models.addModel(properties);
        
        print("just added model... newModel=" + newModel.creatorTokenID);
        print("properties.animationURL=" + properties.animationURL);
        
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
                               radius: modelRadius,
                               modelRotation: palmRotation,
                             };

            debugPrint(">>>>>>>>>>>> EDIT MODEL.... modelRadius=" +modelRadius);

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
        
    if (rightModelAlreadyInHand) {
        var rightTriggerValue = Controller.getTriggerValue(1);
        if (rightTriggerValue > 0.0) {
            var possibleRadius = ((1.0 - rightTriggerValue) * (radiusMaximum - radiusMinimum)) + radiusMinimum;
            modelRadius = possibleRadius;
        } else {
            modelRadius = radiusDefault;
        }
    }

    if (leftModelAlreadyInHand) {
        var leftTriggerValue = Controller.getTriggerValue(0);
        if (leftTriggerValue > 0.0) {
            var possibleRadius = ((1.0 - leftTriggerValue) * (radiusMaximum - radiusMinimum)) + radiusMinimum;
            modelRadius = possibleRadius;
        } else {
            modelRadius = radiusDefault;
        }
    }
}


// register the call back so it fires before each data send
Script.update.connect(checkController);
Controller.keyPressEvent.connect(keyPressEvent);

