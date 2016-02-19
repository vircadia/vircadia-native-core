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
var markerRotation = Quat.fromVec3Degrees({
    x: orientation.x + 10, 
    y: orientation.y - 90,
    z: orientation.z
})
orientation.x = 0;
var whiteboardRotation = Quat.fromVec3Degrees({
    x: 0,
    y: orientation.y - 90,
    z: 0
});
orientation = Quat.fromVec3Degrees(orientation);
var markers = [];


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

createMarkers();
function createMarkers() {
    var modelURLS = [
        "https://s3-us-west-1.amazonaws.com/hifi-content/eric/models/marker-blue.fbx",
        "https://s3-us-west-1.amazonaws.com/hifi-content/eric/models/marker-red.fbx",
        "https://s3-us-west-1.amazonaws.com/hifi-content/eric/models/marker-black.fbx",
    ];

    var markerPosition = Vec3.sum(MyAvatar.position, Vec3.multiply(1.9, Quat.getFront(orientation)));

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
        script: MARKER_SCRIPT_URL,
        userData: JSON.stringify({
            wearable: {
                joints: {
                    RightHand: [{
                        x: 0.001109793782234192,
                        y: 0.13991504907608032,
                        z: 0.05035984516143799
                    }, {
                        x: -0.7360993027687073,
                        y: -0.04330085217952728,
                        z: -0.10863728821277618,
                        w: -0.6666942238807678
                    }],
                    LeftHand: [{
                        x: 0.007193896919488907,
                        y: 0.15147076547145844,
                        z: 0.06174466013908386
                    }, {
                        x: -0.4174973964691162,
                        y: 0.631147563457489,
                        z: -0.3890438377857208,
                        w: -0.52535080909729
                    }]
                }
            }
        })
    });

    markers.push(marker);

    Script.setTimeout(function() {
        var data = {
            whiteboard: whiteboard,
            markerColor: markerColor
        }
        Entities.callEntityMethod(marker, "setProperties", [JSON.stringify(data)]);
    }, 1000)


}



function cleanup() {
    Entities.deleteEntity(whiteboard);
    markers.forEach(function(marker){
        Entities.deleteEntity(marker);
    });
}

Script.scriptEnding.connect(cleanup);