//
//  whiteboardSpawner.js
//  examples/homeContent/whiteboardV2
//
//  Created by Eric Levina on 2/17/16
//  Copyright 2016 High Fidelity, Inc.
//
//  Run this script to spawn a whiteboard, markers, and an eraser.
//  To draw on the whiteboard, equip a marker and hold down trigger with marker tip pointed at whiteboard 
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html

Script.include("../../libraries/utils.js")

var orientation = MyAvatar.orientation;
orientation = Quat.safeEulerAngles(orientation);
var markerRotation = Quat.fromVec3Degrees({
    x: orientation.x + 10,
    y: orientation.y - 90,
    z: orientation.z
})
orientation.x = 0;
var whiteboardRotation = Quat.fromVec3Degrees({
    x: 0,
    y: orientation.y,
    z: 0
});
orientation = Quat.fromVec3Degrees(orientation);
var markers = [];


var whiteboardPosition = Vec3.sum(MyAvatar.position, Vec3.multiply(2, Quat.getFront(orientation)));
var WHITEBOARD_MODEL_URL = "https://s3-us-west-1.amazonaws.com/hifi-content/eric/models/Whiteboard-4.fbx";
var WHITEBOARD_COLLISION_HULL_URL = "https://s3-us-west-1.amazonaws.com/hifi-content/eric/models/whiteboardCollisionHull.obj";
var whiteboard = Entities.addEntity({
    type: "Model",
    name: "whiteboard",
    modelURL: WHITEBOARD_MODEL_URL,
    position: whiteboardPosition,
    rotation: whiteboardRotation,
    shapeType: 'compound',
    compoundShapeURL: WHITEBOARD_COLLISION_HULL_URL,
    dimensions: {
        x: 1.86,
        y: 2.7,
        z: 0.4636
    },
});

var whiteboardSurfacePosition = Vec3.sum(whiteboardPosition, {
    x: 0.0,
    y: 0.45,
    z: 0.0
});
whiteboardSurfacePosition = Vec3.sum(whiteboardSurfacePosition, Vec3.multiply(-0.02, Quat.getRight(whiteboardRotation)));
var moveForwardDistance = 0.02;
whiteboardFrontSurfacePosition = Vec3.sum(whiteboardSurfacePosition, Vec3.multiply(-moveForwardDistance, Quat.getFront(whiteboardRotation)));
var whiteboardSurfaceSettings = {
    type: "Box",
    name: "hifi-whiteboardDrawingSurface",
    dimensions: {
        x: 1.82,
        y: 1.8,
        z: 0.01
    },
    color: {
        red: 200,
        green: 10,
        blue: 200
    },
    position: whiteboardFrontSurfacePosition,
    rotation: whiteboardRotation,
    visible: false,
    parentID: whiteboard
}
var whiteboardFrontDrawingSurface = Entities.addEntity(whiteboardSurfaceSettings);


whiteboardBackSurfacePosition = Vec3.sum(whiteboardSurfacePosition, Vec3.multiply(moveForwardDistance, Quat.getFront(whiteboardRotation)));
whiteboardSurfaceSettings.position = whiteboardBackSurfacePosition;

var whiteboardBackDrawingSurface = Entities.addEntity(whiteboardSurfaceSettings);


var WHITEBOARD_RACK_DEPTH = 1.9;

var ERASER_MODEL_URL = "http://hifi-content.s3.amazonaws.com/alan/dev/eraser-2.fbx";
var ERASER_SCRIPT_URL = Script.resolvePath("eraserEntityScript.js?v43");
var eraserPosition = Vec3.sum(MyAvatar.position, Vec3.multiply(WHITEBOARD_RACK_DEPTH, Quat.getFront(whiteboardRotation)));
eraserPosition = Vec3.sum(eraserPosition, Vec3.multiply(-0.5, Quat.getRight(whiteboardRotation)));
var eraserRotation = markerRotation;

var eraser = Entities.addEntity({
    type: "Model",
    modelURL: ERASER_MODEL_URL,
    position: eraserPosition,
    script: ERASER_SCRIPT_URL,
    shapeType: "box",
    dimensions: {
        x: 0.0858,
        y: 0.0393,
        z: 0.2083
    },
    rotation: eraserRotation,
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
    userData: JSON.stringify({
        originalPosition: eraserPosition,
        originalRotation: eraserRotation,
        wearable: {
            joints: {
                RightHand: [{
                    x: 0.020,
                    y: 0.120,
                    z: 0.049
                }, {
                    x: 0.1004,
                    y: 0.6424,
                    z: 0.717,
                    w: 0.250
                }],
                LeftHand: [{
                    x: -0.005,
                    y: 0.1101,
                    z: 0.053
                }, {
                    x: 0.723,
                    y: 0.289,
                    z: 0.142,
                    w: 0.610
                }]
            }
        }
    })
});

createMarkers();

function createMarkers() {
    var modelURLS = [
        "https://s3-us-west-1.amazonaws.com/hifi-content/eric/models/marker-blue.fbx",
        "https://s3-us-west-1.amazonaws.com/hifi-content/eric/models/marker-red.fbx",
        "https://s3-us-west-1.amazonaws.com/hifi-content/eric/models/marker-black.fbx",
    ];

    var markerPosition = Vec3.sum(MyAvatar.position, Vec3.multiply(WHITEBOARD_RACK_DEPTH, Quat.getFront(orientation)));

    createMarker(modelURLS[0], markerPosition, {
        red: 10,
        green: 10,
        blue: 200
    });

    markerPosition = Vec3.sum(markerPosition, Vec3.multiply(-0.2, Quat.getFront(markerRotation)));
    createMarker(modelURLS[1], markerPosition, {
        red: 200,
        green: 10,
        blue: 10
    });

    markerPosition = Vec3.sum(markerPosition, Vec3.multiply(0.4, Quat.getFront(markerRotation)));
    createMarker(modelURLS[2], markerPosition, {
        red: 10,
        green: 10,
        blue: 10
    });
}


function createMarker(modelURL, markerPosition, markerColor) {
    var MARKER_SCRIPT_URL = Script.resolvePath("markerEntityScript.js?v1" + Math.random());
    var marker = Entities.addEntity({
        type: "Model",
        modelURL: modelURL,
        rotation: markerRotation,
        shapeType: "box",
        name: "marker",
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
            x: 0.027,
            y: 0.027,
            z: 0.164
        },
        name: "marker",
        script: MARKER_SCRIPT_URL,
        userData: JSON.stringify({
            originalPosition: markerPosition,
            originalRotation: markerRotation,
            markerColor: markerColor,
            wearable: {
                joints: {
                    RightHand: [{
                        x: 0.001,
                        y: 0.139,
                        z: 0.050
                    }, {
                        x: -0.73,
                        y: -0.043,
                        z: -0.108,
                        w: -0.666
                    }],
                    LeftHand: [{
                        x: 0.007,
                        y: 0.151,
                        z: 0.061
                    }, {
                        x: -0.417,
                        y: 0.631,
                        z: -0.389,
                        w: -0.525
                    }]
                }
            }
        })
    });

    markers.push(marker);

}



function cleanup() {
    Entities.deleteEntity(whiteboard);
    Entities.deleteEntity(whiteboardFrontDrawingSurface);
    Entities.deleteEntity(whiteboardBackDrawingSurface);
    Entities.deleteEntity(eraser);
    markers.forEach(function(marker) {
        Entities.deleteEntity(marker);
    });
}

Script.scriptEnding.connect(cleanup);