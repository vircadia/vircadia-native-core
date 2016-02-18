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
var MARKER_MODEL_URL = "http://hifi-content.s3.amazonaws.com/alan/dev/marker-blue.fbx";
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
    script: MARKER_SCRIPT_URL,
    userData: JSON.stringify({
        wearable: {
            joints: {
                RightHand: [{
                    "x": 0.03257002681493759,
                    "y": 0.15036098659038544,
                    "z": 0.051217660307884216
                }, {
                    "x": -0.5274277329444885,
                    "y": -0.23446641862392426,
                    "z": -0.05400913953781128,
                    "w": 0.8148180246353149
                }],
                LeftHand: [{
                    "x": -0.031699854880571365,
                    "y": 0.15150733292102814,
                    "z": 0.041107177734375
                }, {
                    "x": 0.649201512336731,
                    "y": 0.1007731482386589,
                    "z": 0.3215889632701874,
                    "w": -0.6818817853927612
                }]
            }
        }
    })
});



function cleanup() {
    Entities.deleteEntity(whiteboard);
    Entities.deleteEntity(marker);
}

Script.scriptEnding.connect(cleanup);