//
//  bot_randomExpression.js
//  examples
//
//  Created by Ben Arnold on 7/23/14.
//  Copyright 2014 High Fidelity, Inc.
//
//  This is an example script that demonstrates an NPC avatar with
//  random facial expressions.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

function getRandomFloat(min, max) {
    return Math.random() * (max - min) + min;
}

function getRandomInt (min, max) {
    return Math.floor(Math.random() * (max - min + 1)) + min;
}

function printVector(string, vector) {
    print(string + " " + vector.x + ", " + vector.y + ", " + vector.z);
}

var timePassed = 0.0;
var updateSpeed = 3.0;

var X_MIN = 5.0;
var X_MAX = 15.0;
var Z_MIN = 5.0;
var Z_MAX = 15.0;
var Y_PELVIS = 1.0;

// pick an integer between 1 and 100 for the body model for this bot
botNumber = getRandomInt(1, 100);

newFaceFilePrefix = "ron";

newBodyFilePrefix = "bot" + botNumber;

// set the face model fst using the bot number
// there is no need to change the body model - we're using the default
Avatar.faceModelURL = Script.getExternalPath(Script.ExternalPaths.Assets, "meshes/" + newFaceFilePrefix + ".fst");
Avatar.skeletonModelURL = Script.getExternalPath(Script.ExternalPaths.Assets, "meshes/" + newBodyFilePrefix + ".fst");
Avatar.billboardURL = Script.getExternalPath(Script.ExternalPaths.Assets, "meshes/billboards/bot" + botNumber + ".png");

Agent.isAvatar = true;
Agent.isListeningToAudioStream = true;

// change the avatar's position to the random one
Avatar.position = { x: getRandomFloat(X_MIN, X_MAX), y: Y_PELVIS, z: getRandomFloat(Z_MIN, Z_MAX) };;  
printVector("New bot, position = ", Avatar.position);

var allBlendShapes = [];
var targetBlendCoefficient = [];
var currentBlendCoefficient = [];

function addBlendShape(s) {
    allBlendShapes[allBlendShapes.length] = s;
}

//It is imperative that the following blendshapes are all present and are in the correct order
addBlendShape("EyeBlink_L");
addBlendShape("EyeBlink_R");
addBlendShape("EyeSquint_L");
addBlendShape("EyeSquint_R");
addBlendShape("EyeDown_L");
addBlendShape("EyeDown_R");
addBlendShape("EyeIn_L");
addBlendShape("EyeIn_R");
addBlendShape("EyeOpen_L");
addBlendShape("EyeOpen_R");
addBlendShape("EyeOut_L");
addBlendShape("EyeOut_R");
addBlendShape("EyeUp_L");
addBlendShape("EyeUp_R");
addBlendShape("BrowsD_L");
addBlendShape("BrowsD_R");
addBlendShape("BrowsU_C");
addBlendShape("BrowsU_L");
addBlendShape("BrowsU_R");
addBlendShape("JawFwd");
addBlendShape("JawLeft");
addBlendShape("JawOpen");
addBlendShape("JawChew");
addBlendShape("JawRight");
addBlendShape("MouthLeft");
addBlendShape("MouthRight");
addBlendShape("MouthFrown_L");
addBlendShape("MouthFrown_R");
addBlendShape("MouthSmile_L");
addBlendShape("MouthSmile_R");
addBlendShape("MouthDimple_L");
addBlendShape("MouthDimple_R");
addBlendShape("LipsStretch_L");
addBlendShape("LipsStretch_R");
addBlendShape("LipsUpperClose");
addBlendShape("LipsLowerClose");
addBlendShape("LipsUpperUp");
addBlendShape("LipsLowerDown");
addBlendShape("LipsUpperOpen");
addBlendShape("LipsLowerOpen");
addBlendShape("LipsFunnel");
addBlendShape("LipsPucker");
addBlendShape("ChinLowerRaise");
addBlendShape("ChinUpperRaise");
addBlendShape("Sneer");
addBlendShape("Puff");
addBlendShape("CheekSquint_L");
addBlendShape("CheekSquint_R");

for (var i = 0; i < allBlendShapes.length; i++) {
    targetBlendCoefficient[i] = 0;
    currentBlendCoefficient[i] = 0;
}

function setRandomExpression() {
    for (var i = 0; i < allBlendShapes.length; i++) {
        targetBlendCoefficient[i] = Math.random();
    }
}

var expressionChangeSpeed = 0.1;

function updateBlendShapes(deltaTime) {
  
    for (var i = 0; i < allBlendShapes.length; i++) {
        currentBlendCoefficient[i] += (targetBlendCoefficient[i] - currentBlendCoefficient[i]) * expressionChangeSpeed;
        Avatar.setBlendshape(allBlendShapes[i], currentBlendCoefficient[i]);
    }
}

function update(deltaTime) {
    timePassed += deltaTime;
    if (timePassed > updateSpeed) {
        timePassed = 0;
        setRandomExpression();
    }
    updateBlendShapes(deltaTime);
}

Script.update.connect(update);