//
//  breakdanceCore.js
//
//  This is the core breakdance game library, it can be used as part of an entity script, or an omniTool module, or bootstapped on it's own
//  Created by Brad Hefta-Gaub on August 24, 2015
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

getPuppetPosition = function(props) {
    var MAX_DISTANCE = 10;
    var searchPosition = MyAvatar.position;

    if (props !== undefined && props.position !== undefined) {
        searchPosition = props.position;
    }

    // first look for the "Table 1" entity
    var ids = Entities.findEntities(searchPosition, MAX_DISTANCE);

    for (var i in ids) {
        var entityId = ids[i];
        var foundProps = Entities.getEntityProperties(entityId);
        if (foundProps.name == "Table 1") {
            // we want to keep the puppet to be bound to the x/z bouds of the table
            // and placed at the top of the table suface. But we want to generally position
            // the puppet at the x/z of where the toy was grabbed.

            var minX = foundProps.boundingBox.brn.x + (props.dimensions.x / 2);
            var maxX = foundProps.boundingBox.tfl.x - (props.dimensions.x / 2);
            var minZ = foundProps.boundingBox.brn.z + (props.dimensions.z / 2);
            var maxZ = foundProps.boundingBox.tfl.z - (props.dimensions.z / 2);

            var bestX = Math.min(maxX, Math.max(props.position.x, minX));
            var bestZ = Math.min(maxZ, Math.max(props.position.z, minZ));

            // Adjust the position to be the "top" of the table. Note: this assumes the table has registrationPoint of 0.5
            // this also assumes the table hasn't been rotated in such a manner that the table top isn't flat
            var bestY = foundProps.boundingBox.center.y + (foundProps.boundingBox.dimensions.y / 2);

            position = { x: bestX, y: bestY, z: bestZ };

            return position;
        }
    }

    if (props !== undefined && props.position !== undefined) {
        return props.position;
    }

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


// some globals we will need access to
var HAND_SIZE = 0.25;
var TARGET_SIZE = 0.3;
var TARGET_COLOR = { red: 128, green: 128, blue: 128};
var TARGET_COLOR_HIT = { red: 0, green: 255, blue: 0};

var textOverlay, leftHandOverlay, rightHandOverlay, 
    leftOnBaseOverlay, leftLoweredOverlay, leftOverheadOverlay, leftSideOverlay, leftFrontOverlay, 
    rightOnBaseOverlay, rightLoweredOverlay, rightOverheadOverlay, rightSideOverlay, rightFrontOverlay;

function createOverlays() {
    textOverlay = Overlays.addOverlay("text", {
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

    leftHandOverlay = Overlays.addOverlay("cube", {
                    position: MyAvatar.getLeftPalmPosition(),
                    size: HAND_SIZE,
                    color: { red: 0, green: 0, blue: 255},
                    alpha: 1,
                    solid: false
                });

    rightHandOverlay = Overlays.addOverlay("cube", {
                    position: MyAvatar.getRightPalmPosition(),
                    size: HAND_SIZE,
                    color: { red: 255, green: 0, blue: 0},
                    alpha: 1,
                    solid: false
                });

    leftOnBaseOverlay = Overlays.addOverlay("cube", {
                    position: getPositionLeftOnBase(),
                    size: TARGET_SIZE,
                    color: TARGET_COLOR,
                    alpha: 1,
                    solid: false
                });


    leftLoweredOverlay = Overlays.addOverlay("cube", {
                    position: getPositionLeftLowered(),
                    size: TARGET_SIZE,
                    color: TARGET_COLOR,
                    alpha: 1,
                    solid: false
                });


    leftOverheadOverlay = Overlays.addOverlay("cube", {
                    position: getPositionLeftOverhead(),
                    size: TARGET_SIZE,
                    color: TARGET_COLOR,
                    alpha: 1,
                    solid: false
                });

    leftSideOverlay = Overlays.addOverlay("cube", {
                    position: getPositionLeftSide(),
                    size: TARGET_SIZE,
                    color: TARGET_COLOR,
                    alpha: 1,
                    solid: false
                });


    leftFrontOverlay = Overlays.addOverlay("cube", {
                    position: getPositionLeftFront(),
                    size: TARGET_SIZE,
                    color: TARGET_COLOR,
                    alpha: 1,
                    solid: false
                });

    rightOnBaseOverlay = Overlays.addOverlay("cube", {
                    position: getPositionRightOnBase(),
                    size: TARGET_SIZE,
                    color: TARGET_COLOR,
                    alpha: 1,
                    solid: false
                });

    rightLoweredOverlay = Overlays.addOverlay("cube", {
                    position: getPositionRightLowered(),
                    size: TARGET_SIZE,
                    color: TARGET_COLOR,
                    alpha: 1,
                    solid: false
                });


    rightOverheadOverlay = Overlays.addOverlay("cube", {
                    position: getPositionRightOverhead(),
                    size: TARGET_SIZE,
                    color: TARGET_COLOR,
                    alpha: 1,
                    solid: false
                });

    rightSideOverlay = Overlays.addOverlay("cube", {
                    position: getPositionRightSide(),
                    size: TARGET_SIZE,
                    color: TARGET_COLOR,
                    alpha: 1,
                    solid: false
                });


    rightFrontOverlay = Overlays.addOverlay("cube", {
                    position: getPositionRightFront(),
                    size: TARGET_SIZE,
                    color: TARGET_COLOR,
                    alpha: 1,
                    solid: false
                });
}

var TEMPORARY_LIFETIME = 60;
var ANIMATION_SETTINGS = {
    url: "http://s3.amazonaws.com/hifi-public/animations/Breakdancing/breakdance_ready.fbx",
    fps: 30,
    running: true,
    loop: true,
    firstFrame: 1,
    lastFrame: 10000
};

var NATURAL_DIMENSIONS =  { x: 1.63, y: 1.67, z: 0.31 };
var DIMENSIONS = Vec3.multiply(NATURAL_DIMENSIONS, 0.3);
var puppetEntityID;

function createPuppet(model, location) {
    if (model === undefined) {
        model = "https://hifi-public.s3.amazonaws.com/models/Bboys/bboy1/bboy1.fbx";
    }
    if (location == undefined) {
        location = getPuppetPosition();
    }
    puppetEntityID = Entities.addEntity({
            type: "Model",
            modelURL: model,
            registrationPoint: { x: 0.5, y: 0, z: 0.5 },
            animation: ANIMATION_SETTINGS,
            position: location,
            collisionless: true,
            dimensions: DIMENSIONS,
            lifetime: TEMPORARY_LIFETIME
        });
}

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


breakdanceStart = function(model, location) {
    print("breakdanceStart...");
    createOverlays();
    createPuppet(model, location);
}

breakdanceUpdate = function(deltaTime) {
    //print("breakdanceUpdate...");

    var leftHandPos = MyAvatar.getLeftPalmPosition();
    var rightHandPos = MyAvatar.getRightPalmPosition();

    Overlays.editOverlay(leftHandOverlay, { position: leftHandPos } );
    Overlays.editOverlay(rightHandOverlay, { position: rightHandPos } );

    var hitTargetLeftOnBase   = findSphereSphereHit(leftHandPos, HAND_SIZE/2, getPositionLeftOnBase(),   TARGET_SIZE/2);
    var hitTargetLeftOverhead = findSphereSphereHit(leftHandPos, HAND_SIZE/2, getPositionLeftOverhead(), TARGET_SIZE/2);
    var hitTargetLeftLowered  = findSphereSphereHit(leftHandPos, HAND_SIZE/2, getPositionLeftLowered(),  TARGET_SIZE/2);
    var hitTargetLeftSide     = findSphereSphereHit(leftHandPos, HAND_SIZE/2, getPositionLeftSide(),     TARGET_SIZE/2);
    var hitTargetLeftFront    = findSphereSphereHit(leftHandPos, HAND_SIZE/2, getPositionLeftFront(),    TARGET_SIZE/2);

    var hitTargetRightOnBase   = findSphereSphereHit(rightHandPos, HAND_SIZE/2, getPositionRightOnBase(),   TARGET_SIZE/2);
    var hitTargetRightOverhead = findSphereSphereHit(rightHandPos, HAND_SIZE/2, getPositionRightOverhead(), TARGET_SIZE/2);
    var hitTargetRightLowered  = findSphereSphereHit(rightHandPos, HAND_SIZE/2, getPositionRightLowered(),  TARGET_SIZE/2);
    var hitTargetRightSide     = findSphereSphereHit(rightHandPos, HAND_SIZE/2, getPositionRightSide(),     TARGET_SIZE/2);
    var hitTargetRightFront    = findSphereSphereHit(rightHandPos, HAND_SIZE/2, getPositionRightFront(),    TARGET_SIZE/2);


    // determine target colors
    var targetColorLeftOnBase   = hitTargetLeftOnBase   ? TARGET_COLOR_HIT : TARGET_COLOR;
    var targetColorLeftOverhead = hitTargetLeftOverhead ? TARGET_COLOR_HIT : TARGET_COLOR;
    var targetColorLeftLowered  = hitTargetLeftLowered  ? TARGET_COLOR_HIT : TARGET_COLOR;
    var targetColorLeftSide     = hitTargetLeftSide     ? TARGET_COLOR_HIT : TARGET_COLOR;
    var targetColorLeftFront    = hitTargetLeftFront    ? TARGET_COLOR_HIT : TARGET_COLOR;

    var targetColorRightOnBase   = hitTargetRightOnBase   ? TARGET_COLOR_HIT : TARGET_COLOR;
    var targetColorRightOverhead = hitTargetRightOverhead ? TARGET_COLOR_HIT : TARGET_COLOR;
    var targetColorRightLowered  = hitTargetRightLowered  ? TARGET_COLOR_HIT : TARGET_COLOR;
    var targetColorRightSide     = hitTargetRightSide     ? TARGET_COLOR_HIT : TARGET_COLOR;
    var targetColorRightFront    = hitTargetRightFront    ? TARGET_COLOR_HIT : TARGET_COLOR;

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
        Overlays.editOverlay(textOverlay, { text: "no pose -- value:" + poseValue });
    } else {
        Overlays.editOverlay(textOverlay, { text: "pose:" + poses[poseValue].name + "\n" + "animation:" + poses[poseValue].animation });
        var props = Entities.getEntityProperties(puppetEntityID);
        //print("puppetEntityID:" + puppetEntityID + "age:"+props.age);
        Entities.editEntity(puppetEntityID, { 
                    animation: { url: poses[poseValue].animation },
                    lifetime: TEMPORARY_LIFETIME + props.age // renew lifetime
                });
    }

    Overlays.editOverlay(leftOnBaseOverlay,   { position: getPositionLeftOnBase(),   color: targetColorLeftOnBase   } );
    Overlays.editOverlay(leftOverheadOverlay, { position: getPositionLeftOverhead(), color: targetColorLeftOverhead } );
    Overlays.editOverlay(leftLoweredOverlay,  { position: getPositionLeftLowered(),  color: targetColorLeftLowered  } );
    Overlays.editOverlay(leftSideOverlay,     { position: getPositionLeftSide()    , color: targetColorLeftSide     } );
    Overlays.editOverlay(leftFrontOverlay,    { position: getPositionLeftFront()   , color: targetColorLeftFront    } );
 
    Overlays.editOverlay(rightOnBaseOverlay,   { position: getPositionRightOnBase(),   color: targetColorRightOnBase   } );
    Overlays.editOverlay(rightOverheadOverlay, { position: getPositionRightOverhead(), color: targetColorRightOverhead } );
    Overlays.editOverlay(rightLoweredOverlay,  { position: getPositionRightLowered(),  color: targetColorRightLowered  } );
    Overlays.editOverlay(rightSideOverlay,     { position: getPositionRightSide()    , color: targetColorRightSide     } );
    Overlays.editOverlay(rightFrontOverlay,    { position: getPositionRightFront()   , color: targetColorRightFront    } );
 }


breakdanceEnd= function() {
    print("breakdanceEnd...");

    Overlays.deleteOverlay(leftHandOverlay);
    Overlays.deleteOverlay(rightHandOverlay);

    Overlays.deleteOverlay(textOverlay);
    Overlays.deleteOverlay(leftOnBaseOverlay);
    Overlays.deleteOverlay(leftOverheadOverlay);
    Overlays.deleteOverlay(leftLoweredOverlay);
    Overlays.deleteOverlay(leftSideOverlay);
    Overlays.deleteOverlay(leftFrontOverlay);
    Overlays.deleteOverlay(rightOnBaseOverlay);
    Overlays.deleteOverlay(rightOverheadOverlay);
    Overlays.deleteOverlay(rightLoweredOverlay);
    Overlays.deleteOverlay(rightSideOverlay);
    Overlays.deleteOverlay(rightFrontOverlay);

    Entities.deleteEntity(puppetEntityID);
}
