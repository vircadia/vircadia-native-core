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
orientation = Quat.fromVec3Degrees(orientation);
var center = Vec3.sum(MyAvatar.position, Vec3.multiply(2, Quat.getFront(orientation)));


var WHITEBOARD_MODEL_URL = "http://hifi-content.s3.amazonaws.com/alan/dev/Whiteboard-3.fbx";
var WHITEBOARD_COLLISION_HULL_URL = "http://hifi-content.s3.amazonaws.com/alan/dev/Whiteboard.obj";
var whiteboard = Entities.addEntity({
    type: "Model",
    modelURL: WHITEBOARD_MODEL_URL,
    position: center,
    shapeType: 'compound',
    compoundShapeURL: WHITEBOARD_COLLISION_HULL_URL,
    dimensions: { x: 0.4636, y: 2.7034, z: 1.8653}
});


function cleanup() {
    Entities.deleteEntity(whiteboard);
}

Script.scriptEnding.connect(cleanup);