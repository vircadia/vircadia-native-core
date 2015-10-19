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


var SCRIPT_URL = Script.resolvePath('bow.js');

var MODEL_URL = "https://hifi-public.s3.amazonaws.com/models/bow/bow.fbx";
var COLLISION_HULL_URL = "https://hifi-public.s3.amazonaws.com/models/bow/bow_collision_hull.obj";
var BOW_DIMENSIONS = {
    x: 0.08,
    y: 0.30,
    z: 0.08
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

var flashlight = Entities.addEntity({
    type: "Model",
    modelURL: MODEL_URL,
    position: center,
    dimensions: BOW_DIMENSIONS,
    collisionsWillMove: true,
    gravity: BOW_GRAVITY,
    shapeType: 'compound',
    script: SCRIPT_URL
});