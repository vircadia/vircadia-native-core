// botProceduralWayPoints.js 

// modified by Adrian McCarlie for worklist job hifi: #19977
// 1 September 2014.
// this script enables a user to define unlimited number of way points on the Domain,
// these points form a path that the bot will follow, once the final way point is reached, 
// the bot will return to the start and continue until script is stopped.
// pause times for each way point can be set individually.
// User must input the x, y, z co-ords for wayPoints[] and time in milliseconds for pauseTimes[]. 
//
// original script
// bot_procedural.js
// hifi
// Created by Ben Arnold on 7/29/2013
//
// Copyright (c) 2014 HighFidelity, Inc. All rights reserved.
//
// This is an example script that demonstrates an NPC avatar.
//
//

//For procedural walk animation
HIFI_PUBLIC_BUCKET = "http://s3.amazonaws.com/hifi-public/";
Script.include(HIFI_PUBLIC_BUCKET + "scripts/acScripts/proceduralAnimationAPI.js");

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

//input co-ords for start position and 7 other positions

var AVATAR_PELVIS_HEIGHT = 0.84;//only change this if you have an odd size avatar

var wayPoints = []; //input locations for all the waypoints
    wayPoints[0] = {x:8131.5, y:202.0, z:8261.5}; //input the location of the start position
    wayPoints[1] = {x: 8160.5, y: 202.0, z: 8261.5}; //input the location of the first way point
    wayPoints[2] = {x: 8160.5, y: 203.0, z: 8270.5}; 
    wayPoints[3] = {x: 8142.5, y: 204.0, z: 8270.5};
    wayPoints[4] = {x: 8142.5, y: 204.0, z: 8272.5};
    wayPoints[5] = {x: 8160.5, y: 203.0, z: 8272.5};
    wayPoints[6] = {x: 8160.5, y: 202.0, z: 8284.5};
    wayPoints[7] = {x: 8111.5, y: 202.0, z: 8284.5};// continue to add locations and add or remove lines as needed.
    
var pauseTimes = []; // the number of pauseTimes must equal the number of wayPoints. Time is in milliseconds
    pauseTimes[0] = 5000; //waiting to go to wayPoint0 (startPoint)
    pauseTimes[1] = 10000; //waiting to go to wayPoint1
    pauseTimes[2] = 3000;
    pauseTimes[3] = 3000;
    pauseTimes[4] = 8000;
    pauseTimes[5] = 6000;
    pauseTimes[6] = 3000;
    pauseTimes[7] = 3000;// add or delete to match way points.

    
wayPoints[0].y = wayPoints[0].y + AVATAR_PELVIS_HEIGHT;
wayPoints[1].y = wayPoints[1].y + AVATAR_PELVIS_HEIGHT;
wayPoints[2].y = wayPoints[2].y + AVATAR_PELVIS_HEIGHT;
wayPoints[3].y = wayPoints[3].y + AVATAR_PELVIS_HEIGHT;
wayPoints[4].y = wayPoints[4].y + AVATAR_PELVIS_HEIGHT;
wayPoints[5].y = wayPoints[5].y + AVATAR_PELVIS_HEIGHT;
wayPoints[6].y = wayPoints[6].y + AVATAR_PELVIS_HEIGHT;
wayPoints[7].y = wayPoints[7].y + AVATAR_PELVIS_HEIGHT;

var CHANCE_OF_MOVING = 0.005;
var CHANCE_OF_SOUND = 0.005;
var CHANCE_OF_HEAD_TURNING = 0.01;
var CHANCE_OF_BIG_MOVE = 1.0;

var isMoving = false;
var isTurningHead = false;
var isPlayingAudio = false; 


var AVATAR_PELVIS_HEIGHT = 1.84;
var MAX_PELVIS_DELTA = 2.5;

var MOVE_RANGE_SMALL = 3.0;
var MOVE_RANGE_BIG = 10.0;
var TURN_RANGE = 70.0;
var STOP_TOLERANCE = 0.05;
var MOVE_RATE = 0.05;
var TURN_RATE = 0.2;
var HEAD_TURN_RATE = 0.05;
var PITCH_RANGE = 15.0;
var YAW_RANGE = 35.0;

var firstPosition = wayPoints[0]; 
var targetPosition =  { x: 0, y: 0, z: 0 };
var targetOrientation = { x: 0, y: 0, z: 0, w: 0 };
var currentOrientation = { x: 0, y: 0, z: 0, w: 0 };
var targetHeadPitch = 0.0;
var targetHeadYaw = 0.0;

var basePelvisHeight = 0.0;
var pelvisOscillatorPosition = 0.0;
var pelvisOscillatorVelocity = 0.0;

function clamp(val, min, max){
    return Math.max(min, Math.min(max, val))
}

//Array of all valid bot numbers
var validBotNumbers = [];

// right now we only use bot 63, since many other bots have messed up skeletons and LOD issues
var botNumber = 63;//getRandomInt(0, 99);

var newFaceFilePrefix = "ron";

var newBodyFilePrefix = "bot" + botNumber;

// set the face model fst using the bot number
// there is no need to change the body model - we're using the default
Avatar.faceModelURL = HIFI_PUBLIC_BUCKET + "meshes/" + newFaceFilePrefix + ".fst";
Avatar.skeletonModelURL = HIFI_PUBLIC_BUCKET + "meshes/" + newBodyFilePrefix + "_a.fst";
Avatar.billboardURL = HIFI_PUBLIC_BUCKET + "meshes/billboards/bot" + botNumber + ".png";

Agent.isAvatar = true;
Agent.isListeningToAudioStream = true;

// change the avatar's position to wayPoints[0]
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

    var footstep_filenames = ["FootstepW2Left-12db.wav", "FootstepW2Right-12db.wav", "FootstepW3Left-12db.wav", "FootstepW3Right-12db.wav", 
                        "FootstepW5Left-12db.wav", "FootstepW5Right-12db.wav"];

    var SOUND_BASE_URL = HIFI_PUBLIC_BUCKET + "sounds/Cocktail+Party+Snippets/Raws/";

    var FOOTSTEP_BASE_URL = HIFI_PUBLIC_BUCKET + "sounds/Footsteps/";

    for (var i = 0; i < sound_filenames.length; i++) {
        sounds.push(SoundCache.getSound(SOUND_BASE_URL + sound_filenames[i]));
    }

    for (var i = 0; i < footstep_filenames.length; i++) {
        footstepSounds.push(SoundCache.getSound(FOOTSTEP_BASE_URL + footstep_filenames[i]));
    }
}

var sounds = [];
var footstepSounds = [];
loadSounds();


function playRandomSound() {
    if (!Agent.isPlayingAvatarSound) {
        var whichSound = Math.floor((Math.random() * sounds.length));
        Agent.playAvatarSound(sounds[whichSound]);
    }
}

function playRandomFootstepSound() {
  var whichSound = Math.floor((Math.random() * footstepSounds.length));  
    Audio.playSound(footstepSounds[whichSound], {
      position: Avatar.position,
    volume: 1.0
    });
}

//  Facial Animation 
var allBlendShapes = [];
var targetBlendCoefficient = [];
var currentBlendCoefficient = [];

//Blendshape constructor
function addBlendshapeToPose(pose, shapeIndex, val) {
    var index = pose.blendShapes.length;
    pose.blendShapes[index] = {shapeIndex: shapeIndex, val: val };
}
//The mood of the avatar, determines face. 0 = happy, 1 = angry, 2 = sad.

//Randomly pick avatar mood. 80% happy, 10% mad 10% sad
var randMood = Math.floor(Math.random() * 11);
var avatarMood;
if (randMood == 0) {
    avatarMood = 1;
} else if (randMood == 2) {
    avatarMood = 2;
} else {
    avatarMood = 0;
}

var currentExpression = -1;
//Face pose constructor
var happyPoses = [];

happyPoses[0] = {blendShapes: []};
addBlendshapeToPose(happyPoses[0], 28, 0.7); //MouthSmile_L
addBlendshapeToPose(happyPoses[0], 29, 0.7); //MouthSmile_R

happyPoses[1] = {blendShapes: []};
addBlendshapeToPose(happyPoses[1], 28, 1.0); //MouthSmile_L
addBlendshapeToPose(happyPoses[1], 29, 1.0); //MouthSmile_R
addBlendshapeToPose(happyPoses[1], 21, 0.2); //JawOpen

happyPoses[2] = {blendShapes: []};
addBlendshapeToPose(happyPoses[2], 28, 1.0); //MouthSmile_L
addBlendshapeToPose(happyPoses[2], 29, 1.0); //MouthSmile_R
addBlendshapeToPose(happyPoses[2], 21, 0.5); //JawOpen
addBlendshapeToPose(happyPoses[2], 46, 1.0); //CheekSquint_L
addBlendshapeToPose(happyPoses[2], 47, 1.0); //CheekSquint_R
addBlendshapeToPose(happyPoses[2], 17, 1.0); //BrowsU_L
addBlendshapeToPose(happyPoses[2], 18, 1.0); //BrowsU_R

var angryPoses = [];

angryPoses[0] = {blendShapes: []};
addBlendshapeToPose(angryPoses[0], 26, 0.6); //MouthFrown_L
addBlendshapeToPose(angryPoses[0], 27, 0.6); //MouthFrown_R
addBlendshapeToPose(angryPoses[0], 14, 0.6); //BrowsD_L
addBlendshapeToPose(angryPoses[0], 15, 0.6); //BrowsD_R

angryPoses[1] = {blendShapes: []};
addBlendshapeToPose(angryPoses[1], 26, 0.9); //MouthFrown_L
addBlendshapeToPose(angryPoses[1], 27, 0.9); //MouthFrown_R
addBlendshapeToPose(angryPoses[1], 14, 0.9); //BrowsD_L
addBlendshapeToPose(angryPoses[1], 15, 0.9); //BrowsD_R

angryPoses[2] = {blendShapes: []};
addBlendshapeToPose(angryPoses[2], 26, 1.0); //MouthFrown_L
addBlendshapeToPose(angryPoses[2], 27, 1.0); //MouthFrown_R
addBlendshapeToPose(angryPoses[2], 14, 1.0); //BrowsD_L
addBlendshapeToPose(angryPoses[2], 15, 1.0); //BrowsD_R
addBlendshapeToPose(angryPoses[2], 21, 0.5); //JawOpen
addBlendshapeToPose(angryPoses[2], 46, 1.0); //CheekSquint_L
addBlendshapeToPose(angryPoses[2], 47, 1.0); //CheekSquint_R

var sadPoses = [];

sadPoses[0] = {blendShapes: []};
addBlendshapeToPose(sadPoses[0], 26, 0.6); //MouthFrown_L
addBlendshapeToPose(sadPoses[0], 27, 0.6); //MouthFrown_R
addBlendshapeToPose(sadPoses[0], 16, 0.2); //BrowsU_C
addBlendshapeToPose(sadPoses[0], 2, 0.6); //EyeSquint_L
addBlendshapeToPose(sadPoses[0], 3, 0.6); //EyeSquint_R

sadPoses[1] = {blendShapes: []};
addBlendshapeToPose(sadPoses[1], 26, 0.9); //MouthFrown_L
addBlendshapeToPose(sadPoses[1], 27, 0.9); //MouthFrown_R
addBlendshapeToPose(sadPoses[1], 16, 0.6); //BrowsU_C
addBlendshapeToPose(sadPoses[1], 2, 0.9); //EyeSquint_L
addBlendshapeToPose(sadPoses[1], 3, 0.9); //EyeSquint_R

sadPoses[2] = {blendShapes: []};
addBlendshapeToPose(sadPoses[2], 26, 1.0); //MouthFrown_L
addBlendshapeToPose(sadPoses[2], 27, 1.0); //MouthFrown_R
addBlendshapeToPose(sadPoses[2], 16, 0.1); //BrowsU_C
addBlendshapeToPose(sadPoses[2], 2, 1.0); //EyeSquint_L
addBlendshapeToPose(sadPoses[2], 3, 1.0); //EyeSquint_R
addBlendshapeToPose(sadPoses[2], 21, 0.3); //JawOpen

var facePoses = [];
facePoses[0] = happyPoses;
facePoses[1] = angryPoses;
facePoses[2] = sadPoses;


function addBlendShape(s) {
    allBlendShapes[allBlendShapes.length] = s;
}

//It is imperative that the following blendshapes are all present and are in the correct order
addBlendShape("EyeBlink_L"); //0
addBlendShape("EyeBlink_R"); //1
addBlendShape("EyeSquint_L"); //2
addBlendShape("EyeSquint_R"); //3
addBlendShape("EyeDown_L"); //4
addBlendShape("EyeDown_R"); //5
addBlendShape("EyeIn_L"); //6
addBlendShape("EyeIn_R"); //7
addBlendShape("EyeOpen_L"); //8
addBlendShape("EyeOpen_R"); //9
addBlendShape("EyeOut_L"); //10
addBlendShape("EyeOut_R"); //11
addBlendShape("EyeUp_L"); //12
addBlendShape("EyeUp_R"); //13
addBlendShape("BrowsD_L"); //14
addBlendShape("BrowsD_R"); //15
addBlendShape("BrowsU_C"); //16
addBlendShape("BrowsU_L"); //17
addBlendShape("BrowsU_R"); //18
addBlendShape("JawFwd"); //19
addBlendShape("JawLeft"); //20
addBlendShape("JawOpen"); //21
addBlendShape("JawChew"); //22
addBlendShape("JawRight"); //23
addBlendShape("MouthLeft"); //24
addBlendShape("MouthRight"); //25
addBlendShape("MouthFrown_L"); //26
addBlendShape("MouthFrown_R"); //27
addBlendShape("MouthSmile_L"); //28
addBlendShape("MouthSmile_R"); //29
addBlendShape("MouthDimple_L"); //30
addBlendShape("MouthDimple_R"); //31
addBlendShape("LipsStretch_L"); //32
addBlendShape("LipsStretch_R"); //33
addBlendShape("LipsUpperClose"); //34
addBlendShape("LipsLowerClose"); //35
addBlendShape("LipsUpperUp"); //36
addBlendShape("LipsLowerDown"); //37
addBlendShape("LipsUpperOpen"); //38
addBlendShape("LipsLowerOpen"); //39
addBlendShape("LipsFunnel"); //40
addBlendShape("LipsPucker"); //41
addBlendShape("ChinLowerRaise"); //42
addBlendShape("ChinUpperRaise"); //43
addBlendShape("Sneer"); //44
addBlendShape("Puff"); //45
addBlendShape("CheekSquint_L"); //46
addBlendShape("CheekSquint_R"); //47

for (var i = 0; i < allBlendShapes.length; i++) {
    targetBlendCoefficient[i] = 0;
    currentBlendCoefficient[i] = 0;
}

function setRandomExpression() {

    //Clear all expression data for current expression
    if (currentExpression != -1) {
        var expression = facePoses[avatarMood][currentExpression];
        for (var i = 0; i < expression.blendShapes.length; i++) {
            targetBlendCoefficient[expression.blendShapes[i].shapeIndex] = 0.0;
        }
    }
    //Get a new current expression
    currentExpression = Math.floor(Math.random() * facePoses[avatarMood].length);
    var expression = facePoses[avatarMood][currentExpression];
    for (var i = 0; i < expression.blendShapes.length; i++) {
        targetBlendCoefficient[expression.blendShapes[i].shapeIndex] = expression.blendShapes[i].val;
    }
}

var expressionChangeSpeed = 0.1;
function updateBlendShapes(deltaTime) {
  
    for (var i = 0; i < allBlendShapes.length; i++) {
        currentBlendCoefficient[i] += (targetBlendCoefficient[i] - currentBlendCoefficient[i]) * expressionChangeSpeed;
        Avatar.setBlendshape(allBlendShapes[i], currentBlendCoefficient[i]);
    }
}

var BLINK_SPEED = 0.15;
var CHANCE_TO_BLINK = 0.0025;
var MAX_BLINK = 0.85;
var blink = 0.0;
var isBlinking = false;
function updateBlinking(deltaTime) {
    if (isBlinking == false) {
        if (Math.random() < CHANCE_TO_BLINK) {
            isBlinking = true;
        } else {
            blink -= BLINK_SPEED;
            if (blink < 0.0) blink = 0.0;
        }
    } else {
        blink += BLINK_SPEED;
        if (blink > MAX_BLINK) {
            blink = MAX_BLINK;
            isBlinking = false;
        }
    }
    
    currentBlendCoefficient[0] = blink;
    currentBlendCoefficient[1] = blink;
    targetBlendCoefficient[0] = blink;
    targetBlendCoefficient[1] = blink;
}

//

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
var JOINT_R_FOOT = 3;
var JOINT_L_FOOT = 8;
var JOINT_R_TOE = 4;
var JOINT_L_TOE = 9;

//  Animation Is Defined Below 

var NUM_FRAMES = 2;
for (var i = 0; i < NUM_FRAMES; i++) {
    rightAngles[i] = [];
    leftAngles[i] = [];
    middleAngles[i] = [];
}
//Joint order for actual joint mappings, should be interleaved R,L,R,L,...S,S,S for R = right, L = left, S = single
var JOINT_ORDER = [];
//*** right / left joints ***
var HIP = 0;
JOINT_ORDER.push(JOINT_R_HIP);
JOINT_ORDER.push(JOINT_L_HIP);
var KNEE = 1;
JOINT_ORDER.push(JOINT_R_KNEE);
JOINT_ORDER.push(JOINT_L_KNEE);
var ARM = 2;
JOINT_ORDER.push(JOINT_R_ARM);
JOINT_ORDER.push(JOINT_L_ARM);
var FOREARM = 3;
JOINT_ORDER.push(JOINT_R_FOREARM);
JOINT_ORDER.push(JOINT_L_FOREARM);
var FOOT = 4;
JOINT_ORDER.push(JOINT_R_FOOT);
JOINT_ORDER.push(JOINT_L_FOOT);
var TOE = 5;
JOINT_ORDER.push(JOINT_R_TOE);
JOINT_ORDER.push(JOINT_L_TOE);
//*** middle joints ***
var SPINE = 0;
JOINT_ORDER.push(JOINT_SPINE);

//We have to store the angles so we can invert yaw and roll when making the animation
//symmetrical

//Front refers to leg, not arm.
//Legs Extending
rightAngles[0][HIP] = [30.0, 0.0, 8.0];
rightAngles[0][KNEE] = [-15.0, 0.0, 0.0];
rightAngles[0][ARM] = [85.0, -25.0, 0.0];
rightAngles[0][FOREARM] = [0.0, 0.0, -15.0];
rightAngles[0][FOOT] = [0.0, 0.0, 0.0];
rightAngles[0][TOE] = [0.0, 0.0, 0.0];

leftAngles[0][HIP] = [-15, 0.0, 8.0];
leftAngles[0][KNEE] = [-26, 0.0, 0.0];
leftAngles[0][ARM] = [85.0, 20.0, 0.0];
leftAngles[0][FOREARM] = [10.0, 0.0, -25.0];
leftAngles[0][FOOT] = [-13.0, 0.0, 0.0];
leftAngles[0][TOE] = [34.0, 0.0, 0.0];

middleAngles[0][SPINE] = [0.0, -15.0, 5.0];

//Legs Passing
rightAngles[1][HIP] = [6.0, 0.0, 8.0];
rightAngles[1][KNEE] = [-12.0, 0.0, 0.0];
rightAngles[1][ARM] = [85.0, 0.0, 0.0];
rightAngles[1][FOREARM] = [0.0, 0.0, -15.0];
rightAngles[1][FOOT] = [6.0, -8.0, 0.0];
rightAngles[1][TOE] = [0.0, 0.0, 0.0];

leftAngles[1][HIP] = [10.0, 0.0, 8.0];
leftAngles[1][KNEE] = [-60.0, 0.0, 0.0];
leftAngles[1][ARM] = [85.0, 0.0, 0.0];
leftAngles[1][FOREARM] = [0.0, 0.0, -15.0];
leftAngles[1][FOOT] = [0.0, 0.0, 0.0];
leftAngles[1][TOE] = [0.0, 0.0, 0.0];

middleAngles[1][SPINE] = [0.0, 0.0, 0.0];

//Actual keyframes for the animation
var walkKeyFrames = procAnimAPI.generateKeyframes(rightAngles, leftAngles, middleAngles, NUM_FRAMES);

//  Animation Is Defined Above

//  Standing Key Frame 
//We don't have to do any mirroring or anything, since this is just a single pose.
var rightQuats = [];
var leftQuats = [];
var middleQuats = [];

rightQuats[HIP] = Quat.fromPitchYawRollDegrees(0.0, 0.0, 7.0);
rightQuats[KNEE] = Quat.fromPitchYawRollDegrees(0.0, 0.0, 0.0);
rightQuats[ARM] = Quat.fromPitchYawRollDegrees(85.0, 0.0, 0.0);
rightQuats[FOREARM] = Quat.fromPitchYawRollDegrees(0.0, 0.0, -10.0);
rightQuats[FOOT] = Quat.fromPitchYawRollDegrees(0.0, -8.0, 0.0);
rightQuats[TOE] = Quat.fromPitchYawRollDegrees(0.0, 0.0, 0.0);

leftQuats[HIP] = Quat.fromPitchYawRollDegrees(0, 0.0, -7.0);
leftQuats[KNEE] = Quat.fromPitchYawRollDegrees(0, 0.0, 0.0);
leftQuats[ARM] = Quat.fromPitchYawRollDegrees(85.0, 0.0, 0.0);
leftQuats[FOREARM] = Quat.fromPitchYawRollDegrees(0.0, 0.0, 10.0);
leftQuats[FOOT] = Quat.fromPitchYawRollDegrees(0.0, 8.0, 0.0);
leftQuats[TOE] = Quat.fromPitchYawRollDegrees(0.0, 0.0, 0.0);

middleQuats[SPINE] = Quat.fromPitchYawRollDegrees(0.0, 0.0, 0.0);

var standingKeyFrame = new procAnimAPI.KeyFrame(rightQuats, leftQuats, middleQuats);

//

var currentFrame = 0;

var walkTime = 0.0;

var walkWheelRadius = 0.5;
var walkWheelRate = 2.0 * 3.141592 * walkWheelRadius / 8.0;

var avatarAcceleration = 0.75;
var avatarVelocity = 0.0;
var avatarMaxVelocity = 1.4;

function handleAnimation(deltaTime) {
  
    updateBlinking(deltaTime);
    updateBlendShapes(deltaTime);
  
    if (Math.random() < 0.01) {
        setRandomExpression();
    }
  
   if (avatarVelocity == 0.0) {
       walkTime = 0.0;
       currentFrame = 0;
   } else {
        walkTime += avatarVelocity * deltaTime;
        if (walkTime > walkWheelRate) {
            walkTime = 0.0;
            currentFrame++;
            if (currentFrame % 2 == 1) {
                playRandomFootstepSound();
            }
            if (currentFrame > 3) {
                currentFrame = 0;
            }
        } 
    }

    var frame = walkKeyFrames[currentFrame];
   
    var walkInterp = walkTime / walkWheelRate;
    var animInterp = avatarVelocity / (avatarMaxVelocity / 1.3);
    if (animInterp > 1.0) animInterp = 1.0;
   
    for (var i = 0; i < JOINT_ORDER.length; i++) {
        var walkJoint = procAnimAPI.deCasteljau(frame.rotations[i], frame.nextFrame.rotations[i], frame.controlPoints[i][0], frame.controlPoints[i][1], walkInterp);
        var standJoint = standingKeyFrame.rotations[i];
        var finalJoint = Quat.mix(standJoint, walkJoint, animInterp);
        Avatar.setJointData(JOINT_ORDER[i], finalJoint);
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
        targetHeadYaw = getRandomFloat(-YAW_RANGE, YAW_RANGE);
        isTurningHead = true;
    } else {
        Avatar.headPitch = Avatar.headPitch + (targetHeadPitch - Avatar.headPitch) * HEAD_TURN_RATE;
        Avatar.headYaw = Avatar.headYaw + (targetHeadYaw - Avatar.headYaw) * HEAD_TURN_RATE;
        if (Math.abs(Avatar.headPitch - targetHeadPitch) < STOP_TOLERANCE && 
            Math.abs(Avatar.headYaw - targetHeadYaw) < STOP_TOLERANCE) {
            isTurningHead = false;
        }
    }
}

function stopWalking() {
    avatarVelocity = 0.0;
    isMoving = false;

}



var pauseTimer;

    function pause(checkPoint, rotation, delay){

        pauseTimer = Script.setTimeout(function() { 
            targetPosition = checkPoint;
            targetOrientation = rotation;
        
            isMoving = true;            
            }, delay);

            
    }


function handleWalking(deltaTime) {

    
    if (!isMoving){ 
        if(targetPosition.x == 0){targetPosition = wayPoints[1]; isMoving = true;} //Start by heading for wayPoint1
        
            else{
                for (var j = 0; j <= wayPoints.length; j++) {  
                if (targetPosition == wayPoints[j]) {   

                if(j == wayPoints.length -1){ j= -1;}
                var k = j + 1;
                var toTarget =  Vec3.normalize(Vec3.subtract(wayPoints[k], Avatar.position));
                    var localVector = Vec3.multiplyQbyV(Avatar.orientation, { x: 0, y: 0, z: -1 });
                    toTarget.y = 0; // I recommend doing that if you don't want weird rotation to occur that are not around Y.

                    var axis = Vec3.normalize(Vec3.cross(toTarget, localVector));
                    var angle = Math.acos(Vec3.dot(toTarget, localVector)) * 180 / Math.PI;

                    if (Vec3.dot(Vec3.cross(axis, localVector), toTarget) < 0) {
                        angle = -angle;
                    }
                    var delta = 1;
        
                        var quat = Quat.angleAxis(angle, axis);
                        Avatar.orientation = Quat.multiply(quat, Avatar.orientation);    
                

                pause(wayPoints[k], Avatar.orientation, pauseTimes[k]);

            
                break;
                }
            }
        }    
    }

    else
    if (isMoving) { 
            
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

    if (AvatarList.containsAvatarWithDisplayName("mrdj")) {
        if (wasMovingLastFrame) {
            isMoving = false;
        }

        // we have a DJ, shouldn't we be dancing?
        jumpWithLoudness(deltaTime);
    } else {

        // no DJ, let's just chill on the dancefloor - randomly walking and talking
        handleHeadTurn();
        handleAnimation(deltaTime);
        handleWalking(deltaTime);
        handleTalking();   
    }
}

Script.update.connect(updateBehavior);

