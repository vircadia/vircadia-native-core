//
//    planky.js
//    examples
//
//    Created by Thijs Wenker on 5/2/14.
//    Copyright 2015 High Fidelity, Inc.
//
//    Pull blocks off the bottom and put them on the top using the grab.js script.
//
//    Distributed under the Apache License, Version 2.0.
//    See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
HIFI_PUBLIC_BUCKET = "http://s3.amazonaws.com/hifi-public/";

const NUM_LAYERS = 16;
const BASE_DIMENSION = { x: 7, y: 2, z: 7 };
const BLOCKS_PER_LAYER = 3;
const BLOCK_SIZE = {x: 0.2, y: 0.1, z: 0.8};
const BLOCK_SPACING = BLOCK_SIZE.x / 3;
const GRAVITY = {x: 0, y: -2.8, z: 0};
const DENSITY = 2000;
const DAMPING_FACTOR = 0.98;
const ANGULAR_DAMPING_FACTOR = 0.8;
const SPAWN_DISTANCE = 3;
const BLOCK_YAW_OFFSET = 45;
const BUTTON_DIMENSIONS = {width: 49, height: 49};

var windowWidth = Window.innerWidth;
var size;
var pieces = [];
var ground = false;
var layerRotated = false;

function grabLowestJointY() {
    var jointNames = MyAvatar.getJointNames();
    var floorY = MyAvatar.position.y;
    for (var jointName in jointNames) {
        if (MyAvatar.getJointPosition(jointNames[jointName]).y < floorY) {
            floorY = MyAvatar.getJointPosition(jointNames[jointName]).y;
        }
    }
    return floorY;
}

function getButtonPosX() {
    return windowWidth - ((BUTTON_DIMENSIONS.width / 2) + BUTTON_DIMENSIONS.width);
}

var button = Overlays.addOverlay('image', {
    x: getButtonPosX(),
    y: 10,
    width: BUTTON_DIMENSIONS.width,
    height: BUTTON_DIMENSIONS.height,
    imageURL: HIFI_PUBLIC_BUCKET + 'marketplace/hificontent/Games/blocks/planky_button.svg',
    alpha: 1
});


function resetBlocks() {
    pieces.forEach(function(piece) {
        Entities.deleteEntity(piece);
    });
    pieces = [];
    var avatarRot = Quat.fromPitchYawRollDegrees(0.0, MyAvatar.bodyYaw, 0.0);
    basePosition = Vec3.sum(MyAvatar.position, Vec3.multiply(SPAWN_DISTANCE, Quat.getFront(avatarRot)));
    basePosition.y = grabLowestJointY() - (BASE_DIMENSION.y / 2);
    if (!ground) {
        ground = Entities.addEntity({
            type: 'Model',
            modelURL: HIFI_PUBLIC_BUCKET + 'eric/models/woodFloor.fbx',
            dimensions: BASE_DIMENSION,
            position: basePosition,
            rotation: avatarRot,
            shapeType: 'box' 
        });
    } else {
        Entities.editEntity(ground, {position: basePosition, rotation: avatarRot});
    }
    var offsetRot = Quat.multiply(avatarRot, Quat.fromPitchYawRollDegrees(0.0, BLOCK_YAW_OFFSET, 0.0));
    basePosition.y += (BASE_DIMENSION.y / 2);
    for (var layerIndex = 0; layerIndex < NUM_LAYERS; layerIndex++) {
        var layerRotated = layerIndex % 2 === 0;
        var offset = -(BLOCK_SPACING);
        var layerRotation = Quat.fromPitchYawRollDegrees(0, layerRotated ? 0 : 90, 0.0);
        for (var blockIndex = 0; blockIndex < BLOCKS_PER_LAYER; blockIndex++) {
            var blockPositionXZ = BLOCK_SIZE.x * blockIndex - (BLOCK_SIZE.x * 3 / 2 - BLOCK_SIZE.x / 2);
            var localTransform = Vec3.multiplyQbyV(offsetRot, {
                x: (layerRotated ? blockPositionXZ + offset: 0),
                y: (BLOCK_SIZE.y / 2) + (BLOCK_SIZE.y * layerIndex),
                z: (layerRotated ? 0 : blockPositionXZ + offset)
            });
            pieces.push(Entities.addEntity({
                type: 'Model',
                modelURL: HIFI_PUBLIC_BUCKET + 'marketplace/hificontent/Games/blocks/block.fbx',
                shapeType: 'box',
                name: 'JengaBlock' + ((layerIndex * BLOCKS_PER_LAYER) + blockIndex),
                dimensions: BLOCK_SIZE,
                position: {
                    x: basePosition.x + localTransform.x,
                    y: basePosition.y + localTransform.y,
                    z: basePosition.z + localTransform.z
                },
                rotation: Quat.multiply(layerRotation, offsetRot),
                collisionsWillMove: true,
                damping: DAMPING_FACTOR,
                angularDamping: ANGULAR_DAMPING_FACTOR,
                gravity: GRAVITY,
                density: DENSITY
           }));
           offset += BLOCK_SPACING;
        }
    }
}

function mousePressEvent(event) {
    var clickedOverlay = Overlays.getOverlayAtPoint({x: event.x, y: event.y});
    if (clickedOverlay === button) {
        resetBlocks();
    }
}

Controller.mousePressEvent.connect(mousePressEvent);
                   
function cleanup() {
    Overlays.deleteOverlay(button);
    if (ground) {
        Entities.deleteEntity(ground);
    }
    pieces.forEach(function(piece) {
        Entities.deleteEntity(piece);
    });
    pieces = [];
}

function onUpdate() {

}

Script.update.connect(onUpdate)
Script.scriptEnding.connect(cleanup);
