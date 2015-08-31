//
// breakdanceToy.js
// examples
//
//  Created by Brad Hefta-Gaub on August 24, 2015
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

HIFI_PUBLIC_BUCKET = "http://s3.amazonaws.com/hifi-public/";

// helpers
// Computes the penetration between a point and a sphere (centered at the origin)
// if point is inside sphere: returns true and stores the result in 'penetration'
// (the vector that would move the point outside the sphere)
// otherwise returns false
function findSphereHit(point, sphereRadius) {
    var EPSILON = 0.000001;	//smallish positive number - used as margin of error for some computations
    var vectorLength = Vec3.length(point);
    if (vectorLength < EPSILON) {
        return true;
    }
    var distance = vectorLength - sphereRadius;
    if (distance < 0.0) {
        return true;
    }
    return false;
}

function findSpherePointHit(sphereCenter, sphereRadius, point) {
    return findSphereHit(Vec3.subtract(point,sphereCenter), sphereRadius);
}

function findSphereSphereHit(firstCenter, firstRadius, secondCenter, secondRadius) {
    return findSpherePointHit(firstCenter, firstRadius + secondRadius, secondCenter);
}


function getPositionPuppet() {
    var DISTANCE_IN_FRONT = 2;
    var DISTANCE_UP = 0.4;
    var DISTANCE_TO_SIDE = 0.0;

    var up = Quat.getUp(MyAvatar.orientation);
    var front = Quat.getFront(MyAvatar.orientation);
    var right = Quat.getRight(MyAvatar.orientation);
    var left = Vec3.multiply(right, -1);

    var upOffset = Vec3.multiply(up, DISTANCE_UP);
    var leftOffset = Vec3.multiply(left, DISTANCE_TO_SIDE);
    var frontOffset = Vec3.multiply(front, DISTANCE_IN_FRONT);

    var offset = Vec3.sum(Vec3.sum(leftOffset, frontOffset), upOffset);
    var position = Vec3.sum(MyAvatar.position, offset);
    return position;
}



function getPositionLeftFront() {
    var DISTANCE_IN_FRONT = 0.6;
    var DISTANCE_UP = 0.4;
    var DISTANCE_TO_SIDE = 0.3;

    var up = Quat.getUp(MyAvatar.orientation);
    var front = Quat.getFront(MyAvatar.orientation);
    var right = Quat.getRight(MyAvatar.orientation);
    var left = Vec3.multiply(right, -1);

    var upOffset = Vec3.multiply(up, DISTANCE_UP);
    var leftOffset = Vec3.multiply(left, DISTANCE_TO_SIDE);
    var frontOffset = Vec3.multiply(front, DISTANCE_IN_FRONT);

    var offset = Vec3.sum(Vec3.sum(leftOffset, frontOffset), upOffset);
    var position = Vec3.sum(MyAvatar.position, offset);
    return position;
}

function getPositionLeftSide() {
    var DISTANCE_IN_FRONT = 0.0;
    var DISTANCE_UP = 0.5;
    var DISTANCE_TO_SIDE = 0.9;

    var up = Quat.getUp(MyAvatar.orientation);
    var front = Quat.getFront(MyAvatar.orientation);
    var right = Quat.getRight(MyAvatar.orientation);
    var left = Vec3.multiply(right, -1);

    var upOffset = Vec3.multiply(up, DISTANCE_UP);
    var leftOffset = Vec3.multiply(left, DISTANCE_TO_SIDE);
    var frontOffset = Vec3.multiply(front, DISTANCE_IN_FRONT);

    var offset = Vec3.sum(Vec3.sum(leftOffset, frontOffset), upOffset);
    var position = Vec3.sum(MyAvatar.position, offset);
    return position;
}

function getPositionLeftOverhead() {
    var DISTANCE_IN_FRONT = 0.2;
    var DISTANCE_UP = 1;
    var DISTANCE_TO_SIDE = 0.3;

    var up = Quat.getUp(MyAvatar.orientation);
    var front = Quat.getFront(MyAvatar.orientation);
    var right = Quat.getRight(MyAvatar.orientation);
    var left = Vec3.multiply(right, -1);

    var upOffset = Vec3.multiply(up, DISTANCE_UP);
    var leftOffset = Vec3.multiply(left, DISTANCE_TO_SIDE);
    var frontOffset = Vec3.multiply(front, DISTANCE_IN_FRONT);

    var offset = Vec3.sum(Vec3.sum(leftOffset, frontOffset), upOffset);
    var position = Vec3.sum(MyAvatar.position, offset);
    return position;
}

function getPositionLeftLowered() {
    var DISTANCE_IN_FRONT = 0.2;
    var DISTANCE_DOWN = 0.1;
    var DISTANCE_TO_SIDE = 0.3;

    var up = Quat.getUp(MyAvatar.orientation);
    var front = Quat.getFront(MyAvatar.orientation);
    var right = Quat.getRight(MyAvatar.orientation);
    var left = Vec3.multiply(right, -1);

    var downOffset = Vec3.multiply(up, DISTANCE_DOWN);
    var leftOffset = Vec3.multiply(left, DISTANCE_TO_SIDE);
    var frontOffset = Vec3.multiply(front, DISTANCE_IN_FRONT);

    var offset = Vec3.sum(Vec3.sum(leftOffset, frontOffset), downOffset );
    var position = Vec3.sum(MyAvatar.position, offset);
    return position;
}

function getPositionLeftOnBase() {
    var DISTANCE_IN_FRONT = 0.2;
    var DISTANCE_DOWN = -0.4;
    var DISTANCE_TO_SIDE = 0.3;

    var up = Quat.getUp(MyAvatar.orientation);
    var front = Quat.getFront(MyAvatar.orientation);
    var right = Quat.getRight(MyAvatar.orientation);
    var left = Vec3.multiply(right, -1);

    var downOffset = Vec3.multiply(up, DISTANCE_DOWN);
    var leftOffset = Vec3.multiply(left, DISTANCE_TO_SIDE);
    var frontOffset = Vec3.multiply(front, DISTANCE_IN_FRONT);

    var offset = Vec3.sum(Vec3.sum(leftOffset, frontOffset), downOffset );
    var position = Vec3.sum(MyAvatar.position, offset);
    return position;
}

function getPositionRightFront() {
    var DISTANCE_IN_FRONT = 0.6;
    var DISTANCE_UP = 0.4;
    var DISTANCE_TO_SIDE = 0.3;

    var up = Quat.getUp(MyAvatar.orientation);
    var front = Quat.getFront(MyAvatar.orientation);
    var right = Quat.getRight(MyAvatar.orientation);

    var upOffset = Vec3.multiply(up, DISTANCE_UP);
    var rightOffset = Vec3.multiply(right, DISTANCE_TO_SIDE);
    var frontOffset = Vec3.multiply(front, DISTANCE_IN_FRONT);

    var offset = Vec3.sum(Vec3.sum(rightOffset, frontOffset), upOffset);
    var position = Vec3.sum(MyAvatar.position, offset);
    return position;
}

function getPositionRightSide() {
    var DISTANCE_IN_FRONT = 0.0;
    var DISTANCE_UP = 0.5;
    var DISTANCE_TO_SIDE = 0.9;

    var up = Quat.getUp(MyAvatar.orientation);
    var front = Quat.getFront(MyAvatar.orientation);
    var right = Quat.getRight(MyAvatar.orientation);

    var upOffset = Vec3.multiply(up, DISTANCE_UP);
    var rightOffset = Vec3.multiply(right, DISTANCE_TO_SIDE);
    var frontOffset = Vec3.multiply(front, DISTANCE_IN_FRONT);

    var offset = Vec3.sum(Vec3.sum(rightOffset, frontOffset), upOffset);
    var position = Vec3.sum(MyAvatar.position, offset);
    return position;
}

function getPositionRightOverhead() {
    var DISTANCE_IN_FRONT = 0.2;
    var DISTANCE_UP = 1;
    var DISTANCE_TO_SIDE = 0.3;

    var up = Quat.getUp(MyAvatar.orientation);
    var front = Quat.getFront(MyAvatar.orientation);
    var right = Quat.getRight(MyAvatar.orientation);

    var upOffset = Vec3.multiply(up, DISTANCE_UP);
    var rightOffset = Vec3.multiply(right, DISTANCE_TO_SIDE);
    var frontOffset = Vec3.multiply(front, DISTANCE_IN_FRONT);

    var offset = Vec3.sum(Vec3.sum(rightOffset, frontOffset), upOffset);
    var position = Vec3.sum(MyAvatar.position, offset);
    return position;
}

function getPositionRightLowered() {
    var DISTANCE_IN_FRONT = 0.2;
    var DISTANCE_DOWN = 0.1;
    var DISTANCE_TO_SIDE = 0.3;

    var up = Quat.getUp(MyAvatar.orientation);
    var front = Quat.getFront(MyAvatar.orientation);
    var right = Quat.getRight(MyAvatar.orientation);

    var downOffset = Vec3.multiply(up, DISTANCE_DOWN);
    var rightOffset = Vec3.multiply(right, DISTANCE_TO_SIDE);
    var frontOffset = Vec3.multiply(front, DISTANCE_IN_FRONT);

    var offset = Vec3.sum(Vec3.sum(rightOffset, frontOffset), downOffset );
    var position = Vec3.sum(MyAvatar.position, offset);
    return position;
}

function getPositionRightOnBase() {
    var DISTANCE_IN_FRONT = 0.2;
    var DISTANCE_DOWN = -0.4;
    var DISTANCE_TO_SIDE = 0.3;

    var up = Quat.getUp(MyAvatar.orientation);
    var front = Quat.getFront(MyAvatar.orientation);
    var right = Quat.getRight(MyAvatar.orientation);

    var downOffset = Vec3.multiply(up, DISTANCE_DOWN);
    var rightOffset = Vec3.multiply(right, DISTANCE_TO_SIDE);
    var frontOffset = Vec3.multiply(front, DISTANCE_IN_FRONT);

    var offset = Vec3.sum(Vec3.sum(rightOffset, frontOffset), downOffset );
    var position = Vec3.sum(MyAvatar.position, offset);
    return position;
}


// We will also demonstrate some 3D overlays. We will create a couple of cubes, spheres, and lines
// our 3D cube that moves around...
var handSize = 0.25;
var leftCubePosition = MyAvatar.getLeftPalmPosition();
var rightCubePosition = MyAvatar.getRightPalmPosition();

var text = Overlays.addOverlay("text", {
                    x: 100,
                    y: 300,
                    width: 900,
                    height: 50,
                    backgroundColor: { red: 0, green: 0, blue: 0},
                    color: { red: 255, green: 255, blue: 255},
                    topMargin: 4,
                    leftMargin: 4,
                    text: "POSE...",
                    alpha: 1,
                    backgroundAlpha: 0.5
                });

var leftHand= Overlays.addOverlay("cube", {
                    position: leftCubePosition,
                    size: handSize,
                    color: { red: 0, green: 0, blue: 255},
                    alpha: 1,
                    solid: false
                });

var rightHand= Overlays.addOverlay("cube", {
                    position: rightCubePosition,
                    size: handSize,
                    color: { red: 255, green: 0, blue: 0},
                    alpha: 1,
                    solid: false
                });


var targetSize = 0.3;
var targetColor = { red: 128, green: 128, blue: 128};
var targetColorHit = { red: 0, green: 255, blue: 0};
var moveCycleColor = { red: 255, green: 255, blue: 0};

var TEMPORARY_LIFETIME = 60;

var animationSettings = JSON.stringify({
    fps: 30,
    running: true,
    loop: true,
    firstFrame: 1,
    lastFrame: 10000
});

var naturalDimensions =  { x: 1.63, y: 1.67, z: 0.31 };
var dimensions = Vec3.multiply(naturalDimensions, 0.3);

var puppetEntityID = Entities.addEntity({
            type: "Model",
            modelURL: "https://hifi-public.s3.amazonaws.com/models/Bboys/bboy1/bboy1.fbx",
            animationURL: "http://s3.amazonaws.com/hifi-public/animations/Breakdancing/breakdance_ready.fbx",
            animationSettings: animationSettings,
            position: getPositionPuppet(),
            ignoreForCollisions: true,
            dimensions: dimensions,
            lifetime: TEMPORARY_LIFETIME
        });

var leftOnBase = Overlays.addOverlay("cube", {
                    position: getPositionLeftOnBase(),
                    size: targetSize,
                    color: targetColor,
                    alpha: 1,
                    solid: false
                });


var leftLowered = Overlays.addOverlay("cube", {
                    position: getPositionLeftLowered(),
                    size: targetSize,
                    color: targetColor,
                    alpha: 1,
                    solid: false
                });


var leftOverhead = Overlays.addOverlay("cube", {
                    position: getPositionLeftOverhead(),
                    size: targetSize,
                    color: targetColor,
                    alpha: 1,
                    solid: false
                });

var leftSide= Overlays.addOverlay("cube", {
                    position: getPositionLeftSide(),
                    size: targetSize,
                    color: targetColor,
                    alpha: 1,
                    solid: false
                });


var leftFront= Overlays.addOverlay("cube", {
                    position: getPositionLeftFront(),
                    size: targetSize,
                    color: targetColor,
                    alpha: 1,
                    solid: false
                });

var rightOnBase = Overlays.addOverlay("cube", {
                    position: getPositionRightOnBase(),
                    size: targetSize,
                    color: targetColor,
                    alpha: 1,
                    solid: false
                });

var rightLowered = Overlays.addOverlay("cube", {
                    position: getPositionRightLowered(),
                    size: targetSize,
                    color: targetColor,
                    alpha: 1,
                    solid: false
                });


var rightOverhead = Overlays.addOverlay("cube", {
                    position: getPositionRightOverhead(),
                    size: targetSize,
                    color: targetColor,
                    alpha: 1,
                    solid: false
                });

var rightSide= Overlays.addOverlay("cube", {
                    position: getPositionRightSide(),
                    size: targetSize,
                    color: targetColor,
                    alpha: 1,
                    solid: false
                });


var rightFront= Overlays.addOverlay("cube", {
                    position: getPositionRightFront(),
                    size: targetSize,
                    color: targetColor,
                    alpha: 1,
                    solid: false
                });

var startDate = new Date();
var lastTime = startDate.getTime();

var NO_POSE         =   0;
var LEFT_ON_BASE    =   1;
var LEFT_OVERHEAD   =   2;
var LEFT_LOWERED    =   4;
var LEFT_SIDE       =   8;
var LEFT_FRONT      =  16;
var RIGHT_ON_BASE   =  32;
var RIGHT_OVERHEAD  =  64;
var RIGHT_LOWERED   = 128;
var RIGHT_SIDE      = 256;
var RIGHT_FRONT     = 512;

var lastPoseValue = NO_POSE;

//http://s3.amazonaws.com/hifi-public/animations/Breakdancing/breakdance_ready.fbx
//http://s3.amazonaws.com/hifi-public/animations/Breakdancing/bboy_pose_to_idle.fbx
//http://s3.amazonaws.com/hifi-public/animations/Breakdancing/bboy_uprock.fbx
//http://s3.amazonaws.com/hifi-public/animations/Breakdancing/bboy_uprock_start.fbx
//http://s3.amazonaws.com/hifi-public/animations/Breakdancing/breakdance_footwork_1.fbx
//http://s3.amazonaws.com/hifi-public/animations/Breakdancing/breakdance_footwork_2.fbx
//http://s3.amazonaws.com/hifi-public/animations/Breakdancing/breakdance_footwork_3.fbx
//http://s3.amazonaws.com/hifi-public/animations/Breakdancing/breakdance_footwork_to_freeze.fbx
//http://s3.amazonaws.com/hifi-public/animations/Breakdancing/breakdance_footwork_to_idle.fbx
//http://s3.amazonaws.com/hifi-public/animations/Breakdancing/breakdance_freeze_var_1.fbx
//http://s3.amazonaws.com/hifi-public/animations/Breakdancing/breakdance_freeze_var_2.fbx
//http://s3.amazonaws.com/hifi-public/animations/Breakdancing/breakdance_freeze_var_3.fbx
//http://s3.amazonaws.com/hifi-public/animations/Breakdancing/breakdance_freeze_var_4.fbx
//http://s3.amazonaws.com/hifi-public/animations/Breakdancing/breakdance_freezes.fbx
//http://s3.amazonaws.com/hifi-public/animations/Breakdancing/breakdance_swipes.fbx
//http://s3.amazonaws.com/hifi-public/animations/Breakdancing/breakdance_uprock.fbx
//http://s3.amazonaws.com/hifi-public/animations/Breakdancing/breakdance_uprock_var_1.fbx
//http://s3.amazonaws.com/hifi-public/animations/Breakdancing/breakdance_uprock_var_1_end.fbx
//http://s3.amazonaws.com/hifi-public/animations/Breakdancing/breakdance_uprock_var_1_start.fbx
//http://s3.amazonaws.com/hifi-public/animations/Breakdancing/breakdance_uprock_var_2.fbx
//http://s3.amazonaws.com/hifi-public/animations/Breakdancing/flair.fbx


var poses = Array();
/*
poses[0                               ] = { name: "no pose",                        animation: "http://s3.amazonaws.com/hifi-public/animations/Breakdancing/breakdance_ready.fbx" };
poses[LEFT_OVERHEAD                   ] = { name: "Left Overhead" ,                 animation: "http://s3.amazonaws.com/hifi-public/animations/Breakdancing/breakdance_ready.fbx" };
poses[LEFT_LOWERED                    ] = { name: "Left Lowered",                   animation: "http://s3.amazonaws.com/hifi-public/animations/Breakdancing/breakdance_ready.fbx" };
poses[LEFT_SIDE                       ] = { name: "Left Side",                      animation: "http://s3.amazonaws.com/hifi-public/animations/Breakdancing/breakdance_ready.fbx" };
poses[LEFT_FRONT                      ] = { name: "Left Front",                     animation: "http://s3.amazonaws.com/hifi-public/animations/Breakdancing/breakdance_ready.fbx" };
poses[RIGHT_OVERHEAD                  ] = { name: "Right Overhead",                 animation: "http://s3.amazonaws.com/hifi-public/animations/Breakdancing/breakdance_ready.fbx" };
poses[RIGHT_LOWERED                   ] = { name: "Right Lowered",                  animation: "http://s3.amazonaws.com/hifi-public/animations/Breakdancing/breakdance_ready.fbx" };
poses[RIGHT_SIDE                      ] = { name: "Right Side",                     animation: "http://s3.amazonaws.com/hifi-public/animations/Breakdancing/breakdance_ready.fbx" };
poses[RIGHT_FRONT                     ] = { name: "Right Front",                    animation: "http://s3.amazonaws.com/hifi-public/animations/Breakdancing/breakdance_ready.fbx" };
*/

poses[LEFT_ON_BASE   + RIGHT_ON_BASE  ] = { name: "Left On Base + Right On Base",   animation: "http://s3.amazonaws.com/hifi-public/animations/Breakdancing/breakdance_ready.fbx" };

poses[LEFT_OVERHEAD  + RIGHT_ON_BASE  ] = { name: "Left Overhead + Right On Base",  animation: "http://s3.amazonaws.com/hifi-public/animations/Breakdancing/breakdance_ready.fbx" };
poses[LEFT_LOWERED   + RIGHT_ON_BASE  ] = { name: "Left Lowered + Right On Base",   animation: "http://s3.amazonaws.com/hifi-public/animations/Breakdancing/breakdance_ready.fbx" };
poses[LEFT_SIDE      + RIGHT_ON_BASE  ] = { name: "Left Side + Right On Base",      animation: "http://s3.amazonaws.com/hifi-public/animations/Breakdancing/breakdance_ready.fbx" };
poses[LEFT_FRONT     + RIGHT_ON_BASE  ] = { name: "Left Front + Right On Base",     animation: "http://s3.amazonaws.com/hifi-public/animations/Breakdancing/breakdance_ready.fbx" };

poses[LEFT_ON_BASE   + RIGHT_OVERHEAD ] = { name: "Left On Base + Right Overhead",  animation: "http://s3.amazonaws.com/hifi-public/animations/Breakdancing/breakdance_ready.fbx" };
poses[LEFT_ON_BASE   + RIGHT_LOWERED  ] = { name: "Left On Base + Right Lowered",   animation: "http://s3.amazonaws.com/hifi-public/animations/Breakdancing/breakdance_ready.fbx" };
poses[LEFT_ON_BASE   + RIGHT_SIDE     ] = { name: "Left On Base + Right Side",      animation: "http://s3.amazonaws.com/hifi-public/animations/Breakdancing/breakdance_ready.fbx" };
poses[LEFT_ON_BASE   + RIGHT_FRONT    ] = { name: "Left On Base + Right Front",     animation: "http://s3.amazonaws.com/hifi-public/animations/Breakdancing/breakdance_ready.fbx" };



poses[LEFT_OVERHEAD  + RIGHT_OVERHEAD ] = { name: "Left Overhead + Right Overhead", animation: "http://s3.amazonaws.com/hifi-public/animations/Breakdancing/bboy_uprock.fbx" };
poses[LEFT_LOWERED   + RIGHT_OVERHEAD ] = { name: "Left Lowered + Right Overhead",  animation: "http://s3.amazonaws.com/hifi-public/animations/Breakdancing/breakdance_footwork_1.fbx" };
poses[LEFT_SIDE      + RIGHT_OVERHEAD ] = { name: "Left Side + Right Overhead",     animation: "http://s3.amazonaws.com/hifi-public/animations/Breakdancing/breakdance_footwork_2.fbx" };
poses[LEFT_FRONT     + RIGHT_OVERHEAD ] = { name: "Left Front + Right Overhead",    animation: "http://s3.amazonaws.com/hifi-public/animations/Breakdancing/breakdance_footwork_3.fbx" };

poses[LEFT_OVERHEAD  + RIGHT_LOWERED  ] = { name: "Left Overhead + Right Lowered",  animation: "http://s3.amazonaws.com/hifi-public/animations/Breakdancing/breakdance_footwork_to_freeze.fbx" };
poses[LEFT_LOWERED   + RIGHT_LOWERED  ] = { name: "Left Lowered + Right Lowered",   animation: "http://s3.amazonaws.com/hifi-public/animations/Breakdancing/breakdance_footwork_to_idle.fbx" };
poses[LEFT_SIDE      + RIGHT_LOWERED  ] = { name: "Left Side + Right Lowered",      animation: "http://s3.amazonaws.com/hifi-public/animations/Breakdancing/breakdance_freeze_var_1.fbx" };
poses[LEFT_FRONT     + RIGHT_LOWERED  ] = { name: "Left Front + Right Lowered",     animation: "http://s3.amazonaws.com/hifi-public/animations/Breakdancing/breakdance_freeze_var_2.fbx" };

poses[LEFT_OVERHEAD  + RIGHT_SIDE     ] = { name: "Left Overhead + Right Side",     animation: "http://s3.amazonaws.com/hifi-public/animations/Breakdancing/breakdance_freeze_var_3.fbx" };
poses[LEFT_LOWERED   + RIGHT_SIDE     ] = { name: "Left Lowered + Right Side",      animation: "http://s3.amazonaws.com/hifi-public/animations/Breakdancing/breakdance_freeze_var_4.fbx" };
poses[LEFT_SIDE      + RIGHT_SIDE     ] = { name: "Left Side + Right Side",         animation: "http://s3.amazonaws.com/hifi-public/animations/Breakdancing/breakdance_freezes.fbx" };
poses[LEFT_FRONT     + RIGHT_SIDE     ] = { name: "Left Front + Right Side",        animation: "http://s3.amazonaws.com/hifi-public/animations/Breakdancing/breakdance_swipes.fbx" };

poses[LEFT_OVERHEAD  + RIGHT_FRONT    ] = { name: "Left Overhead + Right Front",    animation: "http://s3.amazonaws.com/hifi-public/animations/Breakdancing/breakdance_uprock.fbx" };
poses[LEFT_LOWERED   + RIGHT_FRONT    ] = { name: "Left Lowered + Right Front",     animation: "http://s3.amazonaws.com/hifi-public/animations/Breakdancing/breakdance_uprock_var_1.fbx" };
poses[LEFT_SIDE      + RIGHT_FRONT    ] = { name: "Left Side + Right Front",        animation: "http://s3.amazonaws.com/hifi-public/animations/Breakdancing/breakdance_uprock_var_2.fbx" };
poses[LEFT_FRONT     + RIGHT_FRONT    ] = { name: "Left Front + Right Front",       animation: "http://s3.amazonaws.com/hifi-public/animations/Breakdancing/breakdance_uprock_var_1_end.fbx" };


Script.update.connect(function(deltaTime) {
    var date= new Date();
    var now= date.getTime();
    var elapsed = now - lastTime;
    var inMoveCycle = false;

    var leftHandPos = MyAvatar.getLeftPalmPosition();
    var rightHandPos = MyAvatar.getRightPalmPosition();

    Overlays.editOverlay(leftHand, { position: leftHandPos } );
    Overlays.editOverlay(rightHand, { position: rightHandPos } );

    var hitTargetLeftOnBase   = findSphereSphereHit(leftHandPos, handSize/2, getPositionLeftOnBase(),   targetSize/2);
    var hitTargetLeftOverhead = findSphereSphereHit(leftHandPos, handSize/2, getPositionLeftOverhead(), targetSize/2);
    var hitTargetLeftLowered  = findSphereSphereHit(leftHandPos, handSize/2, getPositionLeftLowered(),  targetSize/2);
    var hitTargetLeftSide     = findSphereSphereHit(leftHandPos, handSize/2, getPositionLeftSide(),     targetSize/2);
    var hitTargetLeftFront    = findSphereSphereHit(leftHandPos, handSize/2, getPositionLeftFront(),    targetSize/2);

    var hitTargetRightOnBase   = findSphereSphereHit(rightHandPos, handSize/2, getPositionRightOnBase(),   targetSize/2);
    var hitTargetRightOverhead = findSphereSphereHit(rightHandPos, handSize/2, getPositionRightOverhead(), targetSize/2);
    var hitTargetRightLowered  = findSphereSphereHit(rightHandPos, handSize/2, getPositionRightLowered(),  targetSize/2);
    var hitTargetRightSide     = findSphereSphereHit(rightHandPos, handSize/2, getPositionRightSide(),     targetSize/2);
    var hitTargetRightFront    = findSphereSphereHit(rightHandPos, handSize/2, getPositionRightFront(),    targetSize/2);


    // determine target colors
    var targetColorLeftOnBase   = hitTargetLeftOnBase   ? targetColorHit : targetColor;
    var targetColorLeftOverhead = hitTargetLeftOverhead ? targetColorHit : targetColor;
    var targetColorLeftLowered  = hitTargetLeftLowered  ? targetColorHit : targetColor;
    var targetColorLeftSide     = hitTargetLeftSide     ? targetColorHit : targetColor;
    var targetColorLeftFront    = hitTargetLeftFront    ? targetColorHit : targetColor;

    var targetColorRightOnBase   = hitTargetRightOnBase   ? targetColorHit : targetColor;
    var targetColorRightOverhead = hitTargetRightOverhead ? targetColorHit : targetColor;
    var targetColorRightLowered  = hitTargetRightLowered  ? targetColorHit : targetColor;
    var targetColorRightSide     = hitTargetRightSide     ? targetColorHit : targetColor;
    var targetColorRightFront    = hitTargetRightFront    ? targetColorHit : targetColor;

    // calculate a combined arm pose based on left and right hits
    var poseValue = NO_POSE;
    poseValue += hitTargetLeftOnBase    ? LEFT_ON_BASE    : 0;
    poseValue += hitTargetLeftOverhead  ? LEFT_OVERHEAD   : 0;
    poseValue += hitTargetLeftLowered   ? LEFT_LOWERED    : 0;
    poseValue += hitTargetLeftSide      ? LEFT_SIDE       : 0;
    poseValue += hitTargetLeftFront     ? LEFT_FRONT      : 0;
    poseValue += hitTargetRightOnBase   ? RIGHT_ON_BASE   : 0;
    poseValue += hitTargetRightOverhead ? RIGHT_OVERHEAD  : 0;
    poseValue += hitTargetRightLowered  ? RIGHT_LOWERED   : 0;
    poseValue += hitTargetRightSide     ? RIGHT_SIDE      : 0;
    poseValue += hitTargetRightFront    ? RIGHT_FRONT     : 0;

    if (poses[poseValue] == undefined) {
        Overlays.editOverlay(text, { text: "no pose -- value:" + poseValue });
    } else {
        Overlays.editOverlay(text, { text: "pose:" + poses[poseValue].name + "\n" + "animation:" + poses[poseValue].animation });
        var props = Entities.getEntityProperties(puppetEntityID);
        Entities.editEntity(puppetEntityID, { 
                    animationURL: poses[poseValue].animation,
                    lifetime: TEMPORARY_LIFETIME + props.age // renew lifetime
                });
    }

    lastPoseValue = poseValue;

    Overlays.editOverlay(leftOnBase,   { position: getPositionLeftOnBase(),   color: targetColorLeftOnBase   } );
    Overlays.editOverlay(leftOverhead, { position: getPositionLeftOverhead(), color: targetColorLeftOverhead } );
    Overlays.editOverlay(leftLowered,  { position: getPositionLeftLowered(),  color: targetColorLeftLowered  } );
    Overlays.editOverlay(leftSide,     { position: getPositionLeftSide()    , color: targetColorLeftSide     } );
    Overlays.editOverlay(leftFront,    { position: getPositionLeftFront()   , color: targetColorLeftFront    } );
 
    Overlays.editOverlay(rightOnBase,   { position: getPositionRightOnBase(),   color: targetColorRightOnBase   } );
    Overlays.editOverlay(rightOverhead, { position: getPositionRightOverhead(), color: targetColorRightOverhead } );
    Overlays.editOverlay(rightLowered,  { position: getPositionRightLowered(),  color: targetColorRightLowered  } );
    Overlays.editOverlay(rightSide,     { position: getPositionRightSide()    , color: targetColorRightSide     } );
    Overlays.editOverlay(rightFront,    { position: getPositionRightFront()   , color: targetColorRightFront    } );
 });

Script.scriptEnding.connect(function() {
    Overlays.deleteOverlay(leftHand);
    Overlays.deleteOverlay(rightHand);

    Overlays.deleteOverlay(text);
    Overlays.deleteOverlay(leftOnBase);
    Overlays.deleteOverlay(leftOverhead);
    Overlays.deleteOverlay(leftLowered);
    Overlays.deleteOverlay(leftSide);
    Overlays.deleteOverlay(leftFront);
    Overlays.deleteOverlay(rightOnBase);
    Overlays.deleteOverlay(rightOverhead);
    Overlays.deleteOverlay(rightLowered);
    Overlays.deleteOverlay(rightSide);
    Overlays.deleteOverlay(rightFront);

    print("puppetEntityID:"+puppetEntityID);
    Entities.deleteEntity(puppetEntityID);
});