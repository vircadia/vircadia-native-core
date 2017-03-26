//
//  golfClub.js
//
//  Created by Philip Rosedale on April 11, 2016.
//  Copyright 2016 High Fidelity, Inc.
//
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
//  A simple golf club.  If you have equipped it, and pull trigger, it will either make 
//  you a new golf ball, or take you to your ball if one is not made. 

var orientation = MyAvatar.orientation;
orientation = Quat.safeEulerAngles(orientation);
orientation.x = 0;
orientation = Quat.fromVec3Degrees(orientation);
var center = Vec3.sum(MyAvatar.getHeadPosition(), Vec3.multiply(2, Quat.getForward(orientation)));

var CLUB_MODEL = "http://hifi-production.s3.amazonaws.com/tutorials/golfClub/putter_VR.fbx";
var CLUB_COLLISION_HULL = "http://hifi-production.s3.amazonaws.com/tutorials/golfClub/club_collision_hull.obj";

var CLUB_DIMENSIONS = {
    "x": 0.043093059211969376,
    "y": 1.1488667726516724,
    "z": 0.42455694079399109
};

var CLUB_ROTATION = {
    "w": 0.41972994804382324,
    "x": 0.78570234775543213,
    "y": -0.41875332593917847,
    "z": 0.17653167247772217
};


var SCRIPT_URL = "http://hifi-production.s3.amazonaws.com/tutorials/entity_scripts/golfClub.js";
var golfClubProperties = {
    position: center,
    lifetime: 3600,
    collisionsWillMove: true,
    compoundShapeURL: CLUB_COLLISION_HULL,
    description: "Spawns ball or jumps to ball with trigger",
    dimensions: CLUB_DIMENSIONS,
    dynamic: true,
    modelURL: CLUB_MODEL,
    name: "Tutorial Golf Putter",
    script: SCRIPT_URL,
    shapeType: "compound",
    type: "Model",
    gravity: {
        x: 0,
        y: -5.0,
        z: 0
    },
    userData: JSON.stringify({
        wearable: {
            joints: {
                LeftHand: [{
                    x: -0.1631782054901123,
                    y: 0.44648152589797974,
                    z: 0.10100018978118896
                }, {
                    x: -0.9181621670722961,
                    y: -0.0772884339094162,
                    z: -0.3870723247528076,
                    w: -0.0343472845852375
                }],
                RightHand: [{
                    x: 0.16826771199703217,
                    y: 0.4757269620895386,
                    z: 0.07139724493026733
                }, {
                    x: -0.7976328134536743,
                    y: -0.0011603273451328278,
                    z: 0.6030101776123047,
                    w: -0.012610925361514091
                }]
            }
        }
    })
}


var golfClub = Entities.addEntity(golfClubProperties);

Script.stop();