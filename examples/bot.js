//
//  bot.js
//  hifi
//
//  Created by Stephen Birarda on 2/20/14.
//  Modified by Philip on 2/26/14
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

var CHANCE_OF_MOVING = 0.005; 
var CHANCE_OF_SOUND = 0.005;
var CHANCE_OF_HEAD_TURNING = 0.05;
var CHANCE_OF_BIG_MOVE = 0.1;

var isMoving = false;
var isTurningHead = false;
var isPlayingAudio = false; 

var STARTING_RANGE = 18.0;
var MAX_RANGE = 25.0;
var MOVE_RANGE_SMALL = 0.5;
var MOVE_RANGE_BIG = 5.0;
var TURN_RANGE = 45.0;
var STOP_TOLERANCE = 0.05;
var MOVE_RATE = 0.05;
var TURN_RATE = 0.15;
var PITCH_RATE = 0.20;
var PITCH_RANGE = 30.0;


var firstPosition = { x: getRandomFloat(0, STARTING_RANGE), y: 0, z: getRandomFloat(0, STARTING_RANGE) };
var targetPosition =  { x: 0, y: 0, z: 0 };

var targetDirection = { x: 0, y: 0, z: 0, w: 0 };
var currentDirection = { x: 0, y: 0, z: 0, w: 0 };

var targetHeadPitch = 0.0;

var sounds = [];
sounds.push(new Sound("https://s3-us-west-1.amazonaws.com/highfidelity-public/sounds/Cocktail+Party+Snippets/Raws/AB1.raw"));
sounds.push(new Sound("https://s3-us-west-1.amazonaws.com/highfidelity-public/sounds/Cocktail+Party+Snippets/Raws/Anchorman2.raw"));
sounds.push(new Sound("https://s3-us-west-1.amazonaws.com/highfidelity-public/sounds/Cocktail+Party+Snippets/Raws/B1.raw"));
sounds.push(new Sound("https://s3-us-west-1.amazonaws.com/highfidelity-public/sounds/Cocktail+Party+Snippets/Raws/B1.raw"));
sounds.push(new Sound("https://s3-us-west-1.amazonaws.com/highfidelity-public/sounds/Cocktail+Party+Snippets/Raws/Bale1.raw"));
sounds.push(new Sound("https://s3-us-west-1.amazonaws.com/highfidelity-public/sounds/Cocktail+Party+Snippets/Raws/Bandcamp.raw"));
sounds.push(new Sound("https://s3-us-west-1.amazonaws.com/highfidelity-public/sounds/Cocktail+Party+Snippets/Raws/Big1.raw"));
sounds.push(new Sound("https://s3-us-west-1.amazonaws.com/highfidelity-public/sounds/Cocktail+Party+Snippets/Raws/Big2.raw"));
sounds.push(new Sound("https://s3-us-west-1.amazonaws.com/highfidelity-public/sounds/Cocktail+Party+Snippets/Raws/Brian1.raw"));
sounds.push(new Sound("https://s3-us-west-1.amazonaws.com/highfidelity-public/sounds/Cocktail+Party+Snippets/Raws/Buster1.raw"));
sounds.push(new Sound("https://s3-us-west-1.amazonaws.com/highfidelity-public/sounds/Cocktail+Party+Snippets/Raws/CES1.raw"));
sounds.push(new Sound("https://s3-us-west-1.amazonaws.com/highfidelity-public/sounds/Cocktail+Party+Snippets/Raws/CES2.raw"));
sounds.push(new Sound("https://s3-us-west-1.amazonaws.com/highfidelity-public/sounds/Cocktail+Party+Snippets/Raws/CES3.raw"));
sounds.push(new Sound("https://s3-us-west-1.amazonaws.com/highfidelity-public/sounds/Cocktail+Party+Snippets/Raws/CES4.raw"));
sounds.push(new Sound("https://s3-us-west-1.amazonaws.com/highfidelity-public/sounds/Cocktail+Party+Snippets/Raws/Carrie1.raw"));
sounds.push(new Sound("https://s3-us-west-1.amazonaws.com/highfidelity-public/sounds/Cocktail+Party+Snippets/Raws/Carrie3.raw"));
sounds.push(new Sound("https://s3-us-west-1.amazonaws.com/highfidelity-public/sounds/Cocktail+Party+Snippets/Raws/Charlotte1.raw"));
sounds.push(new Sound("https://s3-us-west-1.amazonaws.com/highfidelity-public/sounds/Cocktail+Party+Snippets/Raws/EN1.raw"));
sounds.push(new Sound("https://s3-us-west-1.amazonaws.com/highfidelity-public/sounds/Cocktail+Party+Snippets/Raws/EN2.raw"));
sounds.push(new Sound("https://s3-us-west-1.amazonaws.com/highfidelity-public/sounds/Cocktail+Party+Snippets/Raws/EN3.raw"));
sounds.push(new Sound("https://s3-us-west-1.amazonaws.com/highfidelity-public/sounds/Cocktail+Party+Snippets/Raws/Eugene1.raw"));
sounds.push(new Sound("https://s3-us-west-1.amazonaws.com/highfidelity-public/sounds/Cocktail+Party+Snippets/Raws/Francesco1.raw"));
sounds.push(new Sound("https://s3-us-west-1.amazonaws.com/highfidelity-public/sounds/Cocktail+Party+Snippets/Raws/Italian1.raw"));
sounds.push(new Sound("https://s3-us-west-1.amazonaws.com/highfidelity-public/sounds/Cocktail+Party+Snippets/Raws/Japanese1.raw"));
sounds.push(new Sound("https://s3-us-west-1.amazonaws.com/highfidelity-public/sounds/Cocktail+Party+Snippets/Raws/Leigh1.raw"));
sounds.push(new Sound("https://s3-us-west-1.amazonaws.com/highfidelity-public/sounds/Cocktail+Party+Snippets/Raws/Lucille1.raw"));
sounds.push(new Sound("https://s3-us-west-1.amazonaws.com/highfidelity-public/sounds/Cocktail+Party+Snippets/Raws/Lucille2.raw"));
sounds.push(new Sound("https://s3-us-west-1.amazonaws.com/highfidelity-public/sounds/Cocktail+Party+Snippets/Raws/MeanGirls.raw"));
sounds.push(new Sound("https://s3-us-west-1.amazonaws.com/highfidelity-public/sounds/Cocktail+Party+Snippets/Raws/Murray2.raw"));
sounds.push(new Sound("https://s3-us-west-1.amazonaws.com/highfidelity-public/sounds/Cocktail+Party+Snippets/Raws/Nigel1.raw"));
sounds.push(new Sound("https://s3-us-west-1.amazonaws.com/highfidelity-public/sounds/Cocktail+Party+Snippets/Raws/PennyLane.raw"));
sounds.push(new Sound("https://s3-us-west-1.amazonaws.com/highfidelity-public/sounds/Cocktail+Party+Snippets/Raws/Pitt1.raw"));
sounds.push(new Sound("https://s3-us-west-1.amazonaws.com/highfidelity-public/sounds/Cocktail+Party+Snippets/Raws/Ricardo.raw"));
sounds.push(new Sound("https://s3-us-west-1.amazonaws.com/highfidelity-public/sounds/Cocktail+Party+Snippets/Raws/SN.raw"));
sounds.push(new Sound("https://s3-us-west-1.amazonaws.com/highfidelity-public/sounds/Cocktail+Party+Snippets/Raws/Sake1.raw"));
sounds.push(new Sound("https://s3-us-west-1.amazonaws.com/highfidelity-public/sounds/Cocktail+Party+Snippets/Raws/Samantha1.raw"));
sounds.push(new Sound("https://s3-us-west-1.amazonaws.com/highfidelity-public/sounds/Cocktail+Party+Snippets/Raws/Samantha2.raw"));
sounds.push(new Sound("https://s3-us-west-1.amazonaws.com/highfidelity-public/sounds/Cocktail+Party+Snippets/Raws/Spicoli1.raw"));
sounds.push(new Sound("https://s3-us-west-1.amazonaws.com/highfidelity-public/sounds/Cocktail+Party+Snippets/Raws/Supernatural.raw"));
sounds.push(new Sound("https://s3-us-west-1.amazonaws.com/highfidelity-public/sounds/Cocktail+Party+Snippets/Raws/Swearengen1.raw"));
sounds.push(new Sound("https://s3-us-west-1.amazonaws.com/highfidelity-public/sounds/Cocktail+Party+Snippets/Raws/TheDude.raw"));
sounds.push(new Sound("https://s3-us-west-1.amazonaws.com/highfidelity-public/sounds/Cocktail+Party+Snippets/Raws/Tony.raw"));
sounds.push(new Sound("https://s3-us-west-1.amazonaws.com/highfidelity-public/sounds/Cocktail+Party+Snippets/Raws/Triumph1.raw"));
sounds.push(new Sound("https://s3-us-west-1.amazonaws.com/highfidelity-public/sounds/Cocktail+Party+Snippets/Raws/Uma1.raw"));
sounds.push(new Sound("https://s3-us-west-1.amazonaws.com/highfidelity-public/sounds/Cocktail+Party+Snippets/Raws/Walken1.raw"));
sounds.push(new Sound("https://s3-us-west-1.amazonaws.com/highfidelity-public/sounds/Cocktail+Party+Snippets/Raws/Walken2.raw"));
sounds.push(new Sound("https://s3-us-west-1.amazonaws.com/highfidelity-public/sounds/Cocktail+Party+Snippets/Raws/Z1.raw"));
sounds.push(new Sound("https://s3-us-west-1.amazonaws.com/highfidelity-public/sounds/Cocktail+Party+Snippets/Raws/Z2.raw"));


//  Play a random sound from a list of conversational audio clips
function audioDone() {
  isPlayingAudio = false;
}

var AVERAGE_AUDIO_LENGTH = 8000;
function playRandomSound(position) {
  if (!isPlayingAudio) {
    var whichSound = Math.floor((Math.random() * sounds.length) % sounds.length);
    var audioOptions = new AudioInjectionOptions();
    audioOptions.volume = 0.25 + (Math.random() * 0.75);
    audioOptions.position = position;
    Audio.playSound(sounds[whichSound], audioOptions);
    isPlayingAudio = true;
    Script.setTimeout(audioDone, AVERAGE_AUDIO_LENGTH);
  }
}

// change the avatar's position to the random one
Avatar.position = firstPosition;  

// pick an integer between 1 and 20 for the face model for this bot
botNumber = getRandomInt(1, 100);

if (botNumber <= 20) {
  newFaceFilePrefix = "bot" + botNumber;
  newBodyFilePrefix = "defaultAvatar_body"
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
  if (Math.random() < CHANCE_OF_SOUND) {
    playRandomSound(Avatar.position);
  }

  if (!isTurningHead && (Math.random() < CHANCE_OF_HEAD_TURNING)) {
    targetHeadPitch = getRandomFloat(-PITCH_RANGE, PITCH_RANGE);
    isTurningHead = true;
  } else {
    Avatar.headPitch = Avatar.headPitch + (targetHeadPitch - Avatar.headPitch) * PITCH_RATE;
    if (Math.abs(Avatar.headPitch - targetHeadPitch) < STOP_TOLERANCE) {
      isTurningHead = false;
    }
  }
  if (!isMoving && (Math.random() < CHANCE_OF_MOVING)) {
    // Set new target location
    targetDirection = Quat.multiply(Avatar.orientation, Quat.angleAxis(getRandomFloat(-TURN_RANGE, TURN_RANGE), { x:0, y:1, z:0 }));
    var front = Quat.getFront(targetDirection);
    if (Math.random() < CHANCE_OF_BIG_MOVE) {
        targetPosition = Vec3.sum(Avatar.position, Vec3.multiply(front, getRandomFloat(0.0, MOVE_RANGE_BIG)));
    } else {
        targetPosition = Vec3.sum(Avatar.position, Vec3.multiply(front, getRandomFloat(0.0, MOVE_RANGE_SMALL)));
    }
    isMoving = true;
  } else { 
    Avatar.position = Vec3.sum(Avatar.position, Vec3.multiply(Vec3.subtract(targetPosition, Avatar.position), MOVE_RATE));
    Avatar.orientation = Quat.mix(Avatar.orientation, targetDirection, TURN_RATE);
    if (Vec3.length(Vec3.subtract(Avatar.position, targetPosition)) < STOP_TOLERANCE) {
       isMoving = false;  
    }
  }
  if (Vec3.length(Avatar.position) > MAX_RANGE) {
    //  Don't let our happy little person get out of the cage
    isMoving = false;
    Avatar.position = { x: 0, y: 0, z: 0 };
  }
}
Script.willSendVisualDataCallback.connect(updateBehavior);
