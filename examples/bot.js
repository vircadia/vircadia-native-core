//
//  bot.js
//  hifi
//
//  Created by Stephen Birarda on 2/20/14.
//  Copyright (c) 2014 HighFidelity, Inc. All rights reserved.
//
//  This is an example script that demonstrates an NPC avatar.
//
//

function getRandomFloat(min, max) {
    return Math.random() * (max - min) + min;
}

function getRandomInt (min, max) {
    return Math.floor(Math.random() * (max - min + 1)) + min;
}

var CHANCE_OF_MOVING = 0.05; 
var isMoving = false;

var STARTING_RANGE = 18.0;
var MOVE_RANGE = 1.0;
var STOP_TOLERANCE = 0.05;
var MOVE_RATE = 0.10;
var firstPosition = { x: getRandomFloat(0, STARTING_RANGE), y: 0, z: getRandomFloat(0, STARTING_RANGE) };
var targetPosition =  { x: 0, y: 0, z: 0 };


// change the avatar's position to the random one
Avatar.position = firstPosition;

// pick an integer between 1 and 20 for the face model for this bot
botNumber = getRandomInt(1, 100);

newBodyFilePrefix = "defaultAvatar";

if (botNumber <= 20) {
  newFaceFilePrefix = "bot" + botNumber;
} else {
  
  if (botNumber <= 40) {
    newFaceFilePrefix = "superhero";
  } else if (botNumber <= 60) {
    newFaceFilePrefix = "amber";
  } else if (botNumber <= 80) {
    newFaceFilePrefix = "ron";
  } else {
    newFaceFilePrefix = "angie";
  }
  
  newBodyFilePrefix = "bot" + botNumber;
} 

// set the face model fst using the bot number
// there is no need to change the body model - we're using the default
Avatar.faceModelURL = "https://s3-us-west-1.amazonaws.com/highfidelity-public/meshes/" + newFaceFilePrefix + ".fst";
Avatar.skeletonModelURL = "https://s3-us-west-1.amazonaws.com/highfidelity-public/meshes/" + newBodyFilePrefix + ".fst";
Avatar.billboardURL = "https://dl.dropboxusercontent.com/u/1864924/bot-billboard.png";

Agent.isAvatar = true;

function updateBehavior() {
  if (!isMoving && (Math.random() < CHANCE_OF_MOVING)) {
    // Set new target location
    targetPosition = {  x: Avatar.position.x + getRandomFloat(-MOVE_RANGE, MOVE_RANGE), 
                        y: Avatar.position.y, 
                        z: Avatar.position.z + getRandomFloat(-MOVE_RANGE, MOVE_RANGE) };
    isMoving = true;
  } else { 

    Avatar.position = Vec3.sum(Avatar.position, Vec3.multiply(Vec3.subtract(targetPosition, Avatar.position), MOVE_RATE));
    if (Vec3.length(Vec3.subtract(Avatar.position, targetPosition)) < STOP_TOLERANCE) {
       isMoving = false;  
    }
  }
}
Script.willSendVisualDataCallback.connect(updateBehavior);
