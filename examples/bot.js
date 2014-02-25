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

// choose a random x and y in the range of 0 to 50
positionX = getRandomFloat(0, 18);
positionZ = getRandomFloat(0, 18);

// change the avatar's position to the random one
Avatar.position = {x: positionX, y: 0, z: positionZ};

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