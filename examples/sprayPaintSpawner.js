//  sprayPaintSpawner.js
//
//  Created by Eric Levin on 9/3/15
//  Copyright 2015 High Fidelity, Inc.
//
//  This is script spwans a spreay paint can model with the sprayPaintCan.js entity script attached
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html

var scriptURL = "https://hifi-public.s3.amazonaws.com/scripts/entityScripts/sprayPaintCan.js";
// var scriptURL = "file:////Users/ericlevin/hifi/examples/entityScripts/sprayPaintCan.js?=v1" + Math.random();
var modelURL = "https://hifi-public.s3.amazonaws.com/eric/models/paintcan.fbx";
var center = Vec3.sum(MyAvatar.position, Vec3.multiply(1, Quat.getFront(Camera.getOrientation())));

var sprayCan = Entities.addEntity({
 type: "Model",
 name: "spraycan",
 modelURL: modelURL,
 position: {x: 549.12, y:495.55, z:503.77},
 rotation: {x: 0, y: 0, z: 0, w: 1},
 dimensions: {
     x: 0.07,
     y: 0.17,
     z: 0.07
 },
 collisionsWillMove: true,
 shapeType: 'box',
 script: scriptURL,
 gravity: {x: 0, y: -0.5, z: 0},
 velocity: {x: 0, y: -.01, z: 0}
});

function cleanup() {
    Entities.deleteEntity(sprayCan);
}

Script.scriptEnding.connect(cleanup);
