//  createParentator.js
//
//  Script Type: Entity Spawner
//  Created by Jeff Moyes on 6/30/2017
//  Copyright 2017 High Fidelity, Inc.
//
//  This script creates a gun-looking item that, when tapped on an entity, and then a second entity, sets the second entity as the paernt of the first
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//


var scriptURL = Script.resolvePath('parentator.js');
var MODEL_URL = Script.resolvePath('resources/Parent-Tool-Production.fbx');
var COLLISION_HULL_URL = Script.resolvePath('resources/Parent-Tool-CollisionHull.obj');
//var COLLISION_SOUND_URL = 'http://hifi-production.s3.amazonaws.com/DomainContent/Toybox/ping_pong_gun/plastic_impact.L.wav';
var START_POSITION = Vec3.sum(Vec3.sum(MyAvatar.position, {
  x: 0,
  y: 0.5,
  z: 0
}), Vec3.multiply(0.7, Quat.getForward(Camera.getOrientation())));
var START_ROTATION = Vec3.sum(MyAvatar.position, Vec3.multiply(1.5, Quat.getFront(Camera.getOrientation())));


var parentator = Entities.addEntity({
    name: "Parent-ator",
    type: "Model",
    modelURL: MODEL_URL,
    shapeType: 'compound',
    compoundShapeURL: COLLISION_HULL_URL,
    dynamic: true,
    script: scriptURL,
    dimensions: {
        x: 0.125,
        y: 0.2875,
        z: 0.5931
    },

    position: START_POSITION,

    rotation: START_ROTATION,


    userData: JSON.stringify({
        "grabbableKey": {"grabbable": true},
        "equipHotspots": [
            {
                "position": {"x": 0.0, "y": 0.0, "z": 0.0},
                "radius": 0.3,
                "joints":{
                    "RightHand":[
                        {"x":0.05, "y":0.3, "z":0.03},
                        {"x":-0.5, "y":-0.5, "z":-0.5, "w":0.5}
                    ],
                    "LeftHand":[
                        {"x":-0.05, "y":0.3, "z":0.03},
                        {"x":-0.5, "y":0.5, "z":0.5, "w":0.5}
                    ]
                }
            }
        ]
    })
});



function cleanUp() {
  Entities.deleteEntity(parentator);
}
Script.scriptEnding.connect(cleanUp);
