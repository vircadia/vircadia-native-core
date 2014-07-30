//
// proceduralBot.js
// hifi
//
// Created by Ben Arnold on 7/29/2013
//
// Copyright (c) 2014 HighFidelity, Inc. All rights reserved.
//
// This is an example script that demonstrates an NPC avatar.
//
//

//For procedural walk animation
Script.include("http://s3-us-west-1.amazonaws.com/highfidelity-public/scripts/proceduralAnimationAPI.js");

var procAnimAPI = new ProcAnimAPI();

function getRandomFloat(min, max) {
    return Math.random() * (max - min) + min;
}

function getRandomInt (min, max) {
    return Math.floor(Math.random() * (max - min + 1)) + min;
}

function printVector(string, vector) {
    print(string + " " + vector.x + ", " + vector.y + ", " + vector.z);
}

var CHANCE_OF_MOVING = 0.1;
var CHANCE_OF_SOUND = 0;
var CHANCE_OF_HEAD_TURNING = 0.05;
var CHANCE_OF_BIG_MOVE = 0.1;

var isMoving = false;
var isTurningHead = false;
var isPlayingAudio = false; 

var X_MIN = 0.50;
var X_MAX = 15.60;
var Z_MIN = 0.50;
var Z_MAX = 15.10;
var Y_PELVIS = 1.0;
var MAX_PELVIS_DELTA = 2.5;

var AVATAR_PELVIS_HEIGHT = 0.75;

var MOVE_RANGE_SMALL = 10.0;
var TURN_RANGE = 70.0;
var STOP_TOLERANCE = 0.05;
var MOVE_RATE = 0.05;
var TURN_RATE = 0.2;
var PITCH_RATE = 0.10;
var PITCH_RANGE = 20.0;

//var firstPosition = { x: getRandomFloat(X_MIN, X_MAX), y: Y_PELVIS, z: getRandomFloat(Z_MIN, Z_MAX) };
var firstPosition = { x: 0.5, y: Y_PELVIS, z: 0.5 };
var targetPosition =  { x: 0, y: 0, z: 0 };
var targetOrientation = { x: 0, y: 0, z: 0, w: 0 };
var currentOrientation = { x: 0, y: 0, z: 0, w: 0 };
var targetHeadPitch = 0.0;

var cumulativeTime = 0.0;

var basePelvisHeight = 0.0;
var pelvisOscillatorPosition = 0.0;
var pelvisOscillatorVelocity = 0.0;

function clamp(val, min, max){
    return Math.max(min, Math.min(max, val))
}

// pick an integer between 1 and 100 that is not 28 for the face model for this bot
botNumber = 28;

while (botNumber == 28) {
  botNumber = getRandomInt(1, 100);
}

if (botNumber <= 20) {
  newFaceFilePrefix = "ron";
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

 newFaceFilePrefix = "ron";
 newBodyFilePrefix = "bot" + 63;

// set the face model fst using the bot number
// there is no need to change the body model - we're using the default
Avatar.faceModelURL = "https://s3-us-west-1.amazonaws.com/highfidelity-public/meshes/" + newFaceFilePrefix + ".fst";
Avatar.skeletonModelURL = "https://s3-us-west-1.amazonaws.com/highfidelity-public/meshes/" + newBodyFilePrefix + "_a.fst";
Avatar.billboardURL = "https://s3-us-west-1.amazonaws.com/highfidelity-public/meshes/billboards/bot" + botNumber + ".png";

Agent.isAvatar = true;
Agent.isListeningToAudioStream = true;

// change the avatar's position to the random one
Avatar.position = firstPosition;  
basePelvisHeight = firstPosition.y; 
printVector("New dancer, position = ", Avatar.position);

function loadSounds() {
  var sound_filenames = ["AB1.raw", "Anchorman2.raw", "B1.raw", "B1.raw", "Bale1.raw", "Bandcamp.raw",
    "Big1.raw", "Big2.raw", "Brian1.raw", "Buster1.raw", "CES1.raw", "CES2.raw", "CES3.raw", "CES4.raw", 
    "Carrie1.raw", "Carrie3.raw", "Charlotte1.raw", "EN1.raw", "EN2.raw", "EN3.raw", "Eugene1.raw", "Francesco1.raw",
    "Italian1.raw", "Japanese1.raw", "Leigh1.raw", "Lucille1.raw", "Lucille2.raw", "MeanGirls.raw", "Murray2.raw",
    "Nigel1.raw", "PennyLane.raw", "Pitt1.raw", "Ricardo.raw", "SN.raw", "Sake1.raw", "Samantha1.raw", "Samantha2.raw", 
    "Spicoli1.raw", "Supernatural.raw", "Swearengen1.raw", "TheDude.raw", "Tony.raw", "Triumph1.raw", "Uma1.raw",
    "Walken1.raw", "Walken2.raw", "Z1.raw", "Z2.raw"
  ];
  
  var SOUND_BASE_URL = "https://s3-us-west-1.amazonaws.com/highfidelity-public/sounds/Cocktail+Party+Snippets/Raws/";
  
  for (var i = 0; i < sound_filenames.length; i++) {
      sounds.push(new Sound(SOUND_BASE_URL + sound_filenames[i]));
  }
}

var sounds = [];
loadSounds();


function playRandomSound() {
  if (!Agent.isPlayingAvatarSound) {
    var whichSound = Math.floor((Math.random() * sounds.length) % sounds.length);
    Agent.playAvatarSound(sounds[whichSound]);
  }
}

//Procedural walk animation using two keyframes
//We use a separate array for front and back joints
//Pitch, yaw, and roll for the joints
var rightAngles = [];
var leftAngles = [];
//for non mirrored joints such as the spine
var middleAngles = [];

//Actual joint mappings
var SHOULDER_JOINT_NUMBER = 15; 
var ELBOW_JOINT_NUMBER = 16;
var JOINT_R_HIP = 1;
var JOINT_R_KNEE = 2;
var JOINT_L_HIP = 6;
var JOINT_L_KNEE = 7;
var JOINT_R_ARM = 15;
var JOINT_R_FOREARM = 16;
var JOINT_L_ARM = 39;
var JOINT_L_FOREARM = 40;
var JOINT_SPINE = 11;

// ******************************* Animation Is Defined Below *************************************

var NUM_FRAMES = 2;
for (var i = 0; i < NUM_FRAMES; i++) {
    rightAngles[i] = [];
    leftAngles[i] = [];
    middleAngles[i] = [];
}
//Joint order for actual joint mappings, should be interleaved R,L,R,L,...S,S,S for R = right, L = left, S = single
var JOINT_ORDER = [JOINT_R_HIP, JOINT_L_HIP, JOINT_R_KNEE, JOINT_L_KNEE, JOINT_R_ARM, JOINT_L_ARM, JOINT_R_FOREARM, JOINT_L_FOREARM, JOINT_SPINE];

//Joint indices for joints that are duplicated, such as arms, It must match JOINT_ORDER
var HIP = 0;
var KNEE = 1;
var ARM = 2;
var FOREARM = 3;
//Joint indices for single joints
var SPINE = 0;

//We have to store the angles so we can invert yaw and roll when making the animation
//symmetrical

//Front refers to leg, not arm.
//Legs Extending
rightAngles[0][HIP] = [30.0, 0.0, 8.0];
rightAngles[0][KNEE] = [-15.0, 0.0, 0.0];
rightAngles[0][ARM] = [85.0, -25.0, 0.0];
rightAngles[0][FOREARM] = [0.0, 0.0, -15.0];

leftAngles[0][HIP] = [-15, 0.0, 8.0];
leftAngles[0][KNEE] = [-28, 0.0, 0.0];
leftAngles[0][ARM] = [85.0, 20.0, 0.0];
leftAngles[0][FOREARM] = [10.0, 0.0, -25.0];

middleAngles[0][SPINE] = [0.0, -15.0, 5.0];

//Legs Passing
rightAngles[1][HIP] = [6.0, 0.0, 8.0];
rightAngles[1][KNEE] = [-12.0, 0.0, 0.0];
rightAngles[1][ARM] = [85.0, 0.0, 0.0];
rightAngles[1][FOREARM] = [0.0, 0.0, -15.0];

leftAngles[1][HIP] = [10.0, 0.0, 8.0];
leftAngles[1][KNEE] = [-55.0, 0.0, 0.0];
leftAngles[1][ARM] = [85.0, 0.0, 0.0];
leftAngles[1][FOREARM] = [0.0, 0.0, -15.0];

middleAngles[1][SPINE] = [0.0, 0.0, 0.0];

// ******************************* Animation Is Defined Above *************************************

//Actual keyframes for the animation
var walkKeyFrames = procAnimAPI.generateKeyframes(rightAngles, leftAngles, middleAngles, NUM_FRAMES);

var currentFrame = 0;

var walkTime = 0.0;

var walkWheelRadius = 0.5;
var walkWheelRate = 2.0 * 3.141592 * walkWheelRadius / 8.0;

var avatarAcceleration = 0.75;
var avatarVelocity = 0.0;
var avatarMaxVelocity = 1.4;

function keepWalking(deltaTime) {
    
  walkTime += avatarVelocity * deltaTime;
  if (walkTime > walkWheelRate) {
    walkTime = 0.0;
    currentFrame++;
    if (currentFrame > 3) {
        currentFrame = 0;
    }
  } 
  
  var frame = walkKeyFrames[currentFrame];
   
  var interp = walkTime / walkWheelRate;
   
  for (var i = 0; i < JOINT_ORDER.length; i++) {
    Avatar.setJointData(JOINT_ORDER[i], procAnimAPI.deCasteljau(frame.rotations[i], frame.nextFrame.rotations[i], frame.controlPoints[i][0], frame.controlPoints[i][1], interp));
  }
}

function jumpWithLoudness(deltaTime) {
  // potentially change pelvis height depending on trailing average loudness
  
  pelvisOscillatorVelocity += deltaTime * Agent.lastReceivedAudioLoudness * 700.0 ;

  pelvisOscillatorVelocity -= pelvisOscillatorPosition * 0.75;
  pelvisOscillatorVelocity *= 0.97;
  pelvisOscillatorPosition += deltaTime * pelvisOscillatorVelocity;
  Avatar.headPitch = pelvisOscillatorPosition * 60.0;

  var pelvisPosition = Avatar.position;
  pelvisPosition.y = (Y_PELVIS - 0.35) + pelvisOscillatorPosition;
  
  if (pelvisPosition.y < Y_PELVIS) {
    pelvisPosition.y = Y_PELVIS;
  } else if (pelvisPosition.y > Y_PELVIS + 1.0) {
    pelvisPosition.y = Y_PELVIS + 1.0;
  }
  
  Avatar.position = pelvisPosition;
}

var forcedMove = false;

var wasMovingLastFrame = false;

function handleHeadTurn() {
  if (!isTurningHead && (Math.random() < CHANCE_OF_HEAD_TURNING)) {
    targetHeadPitch = getRandomFloat(-PITCH_RANGE, PITCH_RANGE);
    isTurningHead = true;
  } else {
    Avatar.headPitch = Avatar.headPitch + (targetHeadPitch - Avatar.headPitch) * PITCH_RATE;
    if (Math.abs(Avatar.headPitch - targetHeadPitch) < STOP_TOLERANCE) {
      isTurningHead = false;
    }
  }
}

function stopWalking() {
  Avatar.clearJointData(JOINT_R_HIP);
  Avatar.clearJointData(JOINT_R_KNEE);
  Avatar.clearJointData(JOINT_L_HIP);
  Avatar.clearJointData(JOINT_L_KNEE);
  avatarVelocity = 0.0;
  isMoving = false;
}

var MAX_ATTEMPTS = 40;
function handleWalking(deltaTime) {

  if (forcedMove || (!isMoving && Math.random() < CHANCE_OF_MOVING)) {
    // Set new target location
    
    //Keep trying new orientations if the desired target location is out of bounds
    var attempts = 0;
    do {
        targetOrientation = Quat.multiply(Avatar.orientation, Quat.angleAxis(getRandomFloat(-TURN_RANGE, TURN_RANGE), { x:0, y:1, z:0 }));
        var front = Quat.getFront(targetOrientation);
        
        targetPosition = Vec3.sum(Avatar.position, Vec3.multiply(front, getRandomFloat(0.0, MOVE_RANGE_SMALL)));
    }
    while ((targetPosition.x < X_MIN || targetPosition.x > X_MAX || targetPosition.z < Z_MIN || targetPosition.z > Z_MAX) 
        && attempts < MAX_ATTEMPTS);
        
    targetPosition.x = clamp(targetPosition.x, X_MIN, X_MAX);
    targetPosition.z = clamp(targetPosition.z, Z_MIN, Z_MAX);
    targetPosition.y = Y_PELVIS;
    
    wasMovingLastFrame = true;
    isMoving = true;
    forcedMove = false;
  } else if (isMoving) { 
    keepWalking(deltaTime);
    
    var targetVector = Vec3.subtract(targetPosition, Avatar.position);
    var distance = Vec3.length(targetVector);
    if (distance <= avatarVelocity * deltaTime) {
        Avatar.position = targetPosition;
        stopWalking(); 
    } else {
        var direction = Vec3.normalize(targetVector);
        //Figure out if we should be slowing down
        var t = avatarVelocity / avatarAcceleration;
        var d = (avatarVelocity / 2.0) * t;
        if (distance < d) { 
            avatarVelocity -= avatarAcceleration * deltaTime;
            if (avatarVelocity <= 0) {
                stopWalking(); 
            }
        } else {    
            avatarVelocity += avatarAcceleration * deltaTime;
            if (avatarVelocity > avatarMaxVelocity) avatarVelocity = avatarMaxVelocity;
        }
        Avatar.position = Vec3.sum(Avatar.position, Vec3.multiply(direction, avatarVelocity * deltaTime));
        Avatar.orientation = Quat.mix(Avatar.orientation, targetOrientation, TURN_RATE);
        
        wasMovingLastFrame = true; 
    
    }
  }
}

function handleTalking() {
  if (Math.random() < CHANCE_OF_SOUND) {
    playRandomSound();
  }
}

function changePelvisHeight(newHeight) {
  var newPosition = Avatar.position;
  newPosition.y = newHeight;
  Avatar.position = newPosition;
}

function updateBehavior(deltaTime) {
  cumulativeTime += deltaTime;

  if (AvatarList.containsAvatarWithDisplayName("mrdj")) {
    if (wasMovingLastFrame) {
      isMoving = false;
    }
    
    // we have a DJ, shouldn't we be dancing?
    jumpWithLoudness(deltaTime);
  } else {
    
    // no DJ, let's just chill on the dancefloor - randomly walking and talking
    handleHeadTurn();
    handleWalking(deltaTime);
    handleTalking();   
  }
}

Script.update.connect(updateBehavior);