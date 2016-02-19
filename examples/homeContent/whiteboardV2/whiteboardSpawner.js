//
//  whiteboardSpawner.js
//  examples/homeContent/whiteboardV2
//
//  Created by Eric Levina on 2/17/16
//  Copyright 2016 High Fidelity, Inc.
//
//  Run this script to spawn a whiteboard and associated acoutrements that one can paint on usignmarkers
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html


var orientation = Camera.getOrientation();
orientation = Quat.safeEulerAngles(orientation);
orientation.x = 0;
var whiteboardRotation = Quat.fromVec3Degrees({
    x: orientation.x,
    y: orientation.y - 90,
    z: orientation.z
});
orientation = Quat.fromVec3Degrees(orientation);


var whiteboardPosition = Vec3.sum(MyAvatar.position, Vec3.multiply(2, Quat.getFront(orientation)));
var WHITEBOARD_MODEL_URL = "http://hifi-content.s3.amazonaws.com/alan/dev/Whiteboard-3.fbx";
var WHITEBOARD_COLLISION_HULL_URL = "http://hifi-content.s3.amazonaws.com/alan/dev/Whiteboard.obj";
var whiteboard = Entities.addEntity({
    type: "Model",
    name: "whiteboard",
    modelURL: WHITEBOARD_MODEL_URL,
    position: whiteboardPosition,
    rotation: whiteboardRotation,
    shapeType: 'compound',
    compoundShapeURL: WHITEBOARD_COLLISION_HULL_URL,
    dimensions: {
        x: 0.4636,
        y: 2.7034,
        z: 1.8653
    }
});

var markerPosition = Vec3.sum(MyAvatar.position, Vec3.multiply(1.9, Quat.getFront(orientation)));
var MARKER_MODEL_URL = "https://s3-us-west-1.amazonaws.com/hifi-content/eric/models/marker-blue.fbx";
var MARKER_SCRIPT_URL = Script.resolvePath("markerEntityScript.js?v1" + Math.random());
var marker = Entities.addEntity({
    type: "Model",
    modelURL: MARKER_MODEL_URL,
    shapeType: "box",
    dynamic: true,
    gravity: {
        x: 0,
        y: -1,
        z: 0
    },
    velocity: {
        x: 0,
        y: -0.1,
        z: 0
    },
    position: markerPosition,
    dimensions: {
        x: 0.0270,
        y: 0.0272,
        z: 0.1641
    },
    name: "marker",
    script: MARKER_SCRIPT_URL
});



function cleanup() {
    Entities.deleteEntity(whiteboard);
    Entities.deleteEntity(marker);
}

Script.scriptEnding.connect(cleanup);