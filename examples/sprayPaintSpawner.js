//  sprayPaintSpawner.js
//
//  Created by Eric Levin on 9/3/15
//  Copyright 2015 High Fidelity, Inc.
//
//  This is script spwans a spreay paint can model with the sprayPaintCan.js entity script attached
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html

//Just temporarily using my own bucket here so others can test the entity. Once PR is tested and merged, then the entity script will appear in its proper place in S3, and I wil switch it
// var scriptURL = "https://hifi-public.s3.amazonaws.com/eric/scripts/sprayPaintCan.js?=v6 ";
var scriptURL = Script.resolvePath("entityScripts/sprayPaintCan.js?v2");
var modelURL = "https://hifi-public.s3.amazonaws.com/eric/models/paintcan.fbx";

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
 velocity: {x: 0, y: -1, z: 0}
});

function cleanup() {

    // Uncomment the below line to delete sprayCan on script reload- for faster iteration during development
    // Entities.deleteEntity(sprayCan);
}

Script.scriptEnding.connect(cleanup);

