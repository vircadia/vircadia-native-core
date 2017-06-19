//
//  Created by Ryan Huffman on 1/10/2017
//  Copyright 2017 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//


var leftHandPosition = {
    "x": 0,
    "y": 0.0559,
    "z": 0.0159
};
var leftHandRotation = Quat.fromPitchYawRollDegrees(90, -90, 0);
var rightHandPosition = Vec3.multiplyVbyV(leftHandPosition, { x: -1, y: 0, z: 0 });
var rightHandRotation = Quat.fromPitchYawRollDegrees(90, 90, 0);

var userData = {
    "grabbableKey": {
        "grabbable": true
    },
    "wearable": {
        "joints": {
            "LeftHand": [
                leftHandPosition,
                leftHandRotation
            ],
            "RightHand": [
                rightHandPosition,
                rightHandRotation
            ]
        }
    }
};

var id = Entities.addEntity({
    "position": MyAvatar.position,
    "collisionsWillMove": 1,
    "compoundShapeURL": Script.resolvePath("models/bow_collision_hull.obj"),
    "created": "2016-09-01T23:57:55Z",
    "dimensions": {
        "x": 0.039999999105930328,
        "y": 1.2999999523162842,
        "z": 0.20000000298023224
    },
    "dynamic": 1,
    "gravity": {
        "x": 0,
        "y": -9.8,
        "z": 0
    },
    "modelURL": Script.resolvePath("models/bow-deadly.baked.fbx"),
    "name": "Hifi-Bow",
    "rotation": {
        "w": 0.9718012809753418,
        "x": 0.15440607070922852,
        "y": -0.10469216108322144,
        "z": -0.14418250322341919
    },
    "script": Script.resolvePath("bow.js"),
    "shapeType": "compound",
    "type": "Model",
    "userData": "{\"grabbableKey\":{\"grabbable\":true},\"wearable\":{\"joints\":{\"RightHand\":[{\"x\":0.0813,\"y\":0.0452,\"z\":0.0095},{\"x\":-0.3946,\"y\":-0.6604,\"z\":0.4748,\"w\":-0.4275}],\"LeftHand\":[{\"x\":-0.0881,\"y\":0.0259,\"z\":0.0159},{\"x\":0.4427,\"y\":-0.6519,\"z\":0.4592,\"w\":0.4099}]}}}",
    "lifetime": 600
});
print("Created bow:", id);
