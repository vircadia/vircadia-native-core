//
//  createBow.js
//
//  Created byJames Pollack @imgntn on 10/19/2015
//  Copyright 2015 High Fidelity, Inc.
//
//  This script creates a bow you can use to shoot an arrow.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

var utilsPath = Script.resolvePath('../libraries/utils.js');
Script.include(utilsPath);

var SCRIPT_URL = Script.resolvePath('bow.js');

var MODEL_URL = "https://cdn-1.vircadia.com/us-e-1/DomainContent/Toybox/bow/bow-deadly.fbx";
var COLLISION_HULL_URL = "https://cdn-1.vircadia.com/us-e-1/DomainContent/Toybox/bow/bow_collision_hull.obj";
var BOW_DIMENSIONS = {
    x: 0.04,
    y: 1.3,
    z: 0.21
};

var BOW_GRAVITY = {
    x: 0,
    y: 0,
    z: 0
}

var center = Vec3.sum(Vec3.sum(MyAvatar.position, {
    x: 0,
    y: 0.5,
    z: 0
}), Vec3.multiply(1, Quat.getFront(Camera.getOrientation())));


var TOP_NOTCH_OFFSET = 0.6;

var BOTTOM_NOTCH_OFFSET = 0.6;

var LINE_DIMENSIONS = {
    x: 5,
    y: 5,
    z: 5
};

var bow;


function makeBow() {

    var bowProperties = {
        name: 'Hifi-Bow',
        type: "Model",
        modelURL: MODEL_URL,
        position: center,
        dimensions: BOW_DIMENSIONS,
        dynamic: true,
        gravity: BOW_GRAVITY,
        shapeType: 'compound',
        compoundShapeURL: COLLISION_HULL_URL,
        script: SCRIPT_URL,
        collidesWith: 'dynamic,kinematic,static',
        userData: JSON.stringify({
            grabbableKey: {
                invertSolidWhileHeld: true
            },
            wearable: {
                joints: {
                    RightHand: [{
                        x: 0.03960523009300232,
                        y: 0.01979270577430725,
                        z: 0.03294898942112923
                    }, {
                        x: -0.7257906794548035,
                        y: -0.4611682891845703,
                        z: 0.4436084032058716,
                        w: -0.25251442193984985
                    }],
                    LeftHand: [{
                        x: 0.0055799782276153564,
                        y: 0.04354757443070412,
                        z: 0.05119767785072327
                    }, {
                        x: -0.14914104342460632,
                        y: 0.6448180079460144,
                        z: -0.2888556718826294,
                        w: -0.6917579770088196
                    }]
                }
            }
        })
    };

    bow = Entities.addEntity(bowProperties);
    createPreNotchString();
}

var preNotchString;

function createPreNotchString() {

    var bowProperties = Entities.getEntityProperties(bow, ["position", "rotation", "userData"]);
    var downVector = Vec3.multiply(-1, Quat.getUp(bowProperties.rotation));
    var downOffset = Vec3.multiply(downVector, BOTTOM_NOTCH_OFFSET * 2);
    var upVector = Quat.getUp(bowProperties.rotation);
    var upOffset = Vec3.multiply(upVector, TOP_NOTCH_OFFSET);

    var backOffset = Vec3.multiply(-0.1, Quat.getFront(bowProperties.rotation));
    var topStringPosition = Vec3.sum(bowProperties.position, upOffset);
    topStringPosition = Vec3.sum(topStringPosition, backOffset);

    var stringProperties = {
        name: 'Hifi-Bow-Pre-Notch-String',
        type: 'Line',
        position: topStringPosition,
        linePoints: [{
            x: 0,
            y: 0,
            z: 0
        }, Vec3.sum({
            x: 0,
            y: 0,
            z: 0
        }, downOffset)],
        color: {
            red: 255,
            green: 255,
            blue: 255
        },
        dimensions: LINE_DIMENSIONS,
        visible: true,
        dynamic: false,
        collisionless: true,
        parentID: bow,
        userData: JSON.stringify({
            grabbableKey: {
                grabbable: false
            }
        })
    };

    preNotchString = Entities.addEntity(stringProperties);

    var data = {
        preNotchString: preNotchString
    };

    setEntityCustomData('bowKey', bow, data);
}

makeBow();

function cleanup() {
    Entities.deleteEntity(bow);
    Entities.deleteEntity(preNotchString);
}

Script.scriptEnding.connect(cleanup);