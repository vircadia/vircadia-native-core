var leftHandPosition = {
    "x": 0,//-0.0881,
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
}

Entities.addEntity({
    position: MyAvatar.position,
    "collisionsWillMove": 1,
    "compoundShapeURL": Script.resolvePath("bow_collision_hull.obj"),
    "created": "2016-09-01T23:57:55Z",
    "dimensions": {
        "x": 0.039999999105930328,
        "y": 1.2999999523162842,
        "z": 0.20000000298023224
    },
    "dynamic": 1,
    "gravity": {
        "x": 0,
        "y": -1,
        "z": 0
    },
    "modelURL": Script.resolvePath("bow-deadly.fbx"),
    "name": "Hifi-Bow",
    "rotation": {
        "w": 0.9718012809753418,
        "x": 0.15440607070922852,
        "y": -0.10469216108322144,
        "z": -0.14418250322341919
    },
    "script": Script.resolvePath("bow.js") + "?" + Date.now(),
    "shapeType": "compound",
    "type": "Model",
    "userData": JSON.stringify(userData),
    lifetime: 600
});
