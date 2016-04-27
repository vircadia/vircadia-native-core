//
//  lodTest.js
//  examples/tests
//
//  Created by Ryan Huffman on 11/19/15.
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

var MIN_DIM = 0.001;
var MAX_DIM = 2.0;
var NUM_SPHERES = 20;

// Rough estimate of the width the spheres will span, not taking into account MIN_DIM
var WIDTH = MAX_DIM * NUM_SPHERES;

var entities = [];
var right = Quat.getRight(Camera.orientation);
// Starting position will be 30 meters in front of the camera
var position = Vec3.sum(Camera.position, Vec3.multiply(30, Quat.getFront(Camera.orientation)));
position = Vec3.sum(position, Vec3.multiply(-WIDTH/2, right));

for (var i = 0; i < NUM_SPHERES; ++i) {
    var dim = (MAX_DIM - MIN_DIM) * ((i + 1) / NUM_SPHERES);
    entities.push(Entities.addEntity({
        type: "Sphere",
        dimensions: { x: dim, y: dim, z: dim },
        position: position,
    }));

    position = Vec3.sum(position, Vec3.multiply(dim * 2, right));
}

Script.scriptEnding.connect(function() {
    for (var i = 0; i < entities.length; ++i) {
        Entities.deleteEntity(entities[i]);
    }
})
