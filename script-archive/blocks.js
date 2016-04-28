// 
//  Blocks.js 
//  
//  Created by Philip Rosedale on January 26, 2015
//  Copyright 2015 High Fidelity, Inc.
//  
//  Create a bunch of building blocks and drop them onto a playing surface in front of you. 
// 
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
var FLOOR_SIZE = 7.5;
var FLOOR_THICKNESS = 0.10;
var EDGE_THICKESS = 0.25;
var SCALE = 0.25; 

var NUM_BLOCKS = 25; 
var DROP_HEIGHT = SCALE * 8.0;

var GRAVITY = -1.0;
var LIFETIME = 6000;    
var DAMPING = 0.50;

var blockTypes = [];
blockTypes.push({ x: 1, y: 1, z: 1, red: 255, green: 0, blue: 0 });
blockTypes.push({ x: 1, y: 1, z: 2, red: 0, green: 255, blue: 0 });
blockTypes.push({ x: 1, y: 2, z: 5, red: 0, green: 0, blue: 255 });
blockTypes.push({ x: 1, y: 2, z: 2, red: 255, green: 255, blue: 0 });
blockTypes.push({ x: 1, y: 1, z: 5, red: 0, green: 255, blue: 255 });

var center = Vec3.sum(MyAvatar.position, Vec3.multiply(FLOOR_SIZE * 2.0, Quat.getFront(Camera.getOrientation())));

var floor = Entities.addEntity(
            { type: "Box",
            position: Vec3.subtract(center, { x: 0, y: SCALE / 2.0, z: 0 }), 
            dimensions: { x: FLOOR_SIZE, y: FLOOR_THICKNESS, z: FLOOR_SIZE }, 
              color: { red: 128, green: 128, blue: 128 },
              gravity: {  x: 0, y: 0, z: 0 },
            ignoreCollisions: false,
            locked: true,
            lifetime: LIFETIME });

var edge1 = Entities.addEntity(
            { type: "Box",
            position: Vec3.sum(center, { x: FLOOR_SIZE / 2.0, y: FLOOR_THICKNESS / 2.0, z: 0 }), 
            dimensions: { x: EDGE_THICKESS, y: EDGE_THICKESS, z: FLOOR_SIZE + EDGE_THICKESS }, 
              color: { red: 100, green: 100, blue: 100 },
              gravity: {  x: 0, y: 0, z: 0 },
            ignoreCollisions: false,
            visible: true,
            locked: true,
            lifetime: LIFETIME });

var edge2 = Entities.addEntity(
            { type: "Box",
            position: Vec3.sum(center, { x: -FLOOR_SIZE / 2.0, y: FLOOR_THICKNESS / 2.0, z: 0 }), 
            dimensions: { x: EDGE_THICKESS, y: EDGE_THICKESS, z: FLOOR_SIZE + EDGE_THICKESS }, 
              color: { red: 100, green: 100, blue: 100 },
              gravity: {  x: 0, y: 0, z: 0 },
            ignoreCollisions: false,
            visible: true,
            locked: true,
            lifetime: LIFETIME });

var edge3 = Entities.addEntity(
            { type: "Box",
            position: Vec3.sum(center, { x: 0, y: FLOOR_THICKNESS / 2.0, z: -FLOOR_SIZE / 2.0 }), 
            dimensions: { x: FLOOR_SIZE + EDGE_THICKESS, y: EDGE_THICKESS, z: EDGE_THICKESS }, 
              color: { red: 100, green: 100, blue: 100 },
              gravity: {  x: 0, y: 0, z: 0 },
            ignoreCollisions: false,
            visible: true,
            locked: true,
            lifetime: LIFETIME });

var edge4 = Entities.addEntity(
            { type: "Box",
            position: Vec3.sum(center, { x: 0, y: FLOOR_THICKNESS / 2.0, z: FLOOR_SIZE / 2.0 }), 
            dimensions: { x: FLOOR_SIZE + EDGE_THICKESS, y: EDGE_THICKESS, z: EDGE_THICKESS }, 
              color: { red: 100, green: 100, blue: 100 },
              gravity: {  x: 0, y: 0, z: 0 },
            ignoreCollisions: false,
            visible: true,
            locked: true,
            lifetime: LIFETIME });

blocks = [];

for (var i = 0; i < NUM_BLOCKS; i++) {
    var which = Math.floor(Math.random() * blockTypes.length);
    var type = blockTypes[which];  
    blocks.push(Entities.addEntity(
            { type: "Box",
            position: { x: center.x + (Math.random() - 0.5) * (FLOOR_SIZE * 0.75), 
                        y: center.y + DROP_HEIGHT, 
                        z: center.z + (Math.random() - 0.5) * (FLOOR_SIZE * 0.75) },  
            dimensions: { x: type.x * SCALE, y: type.y * SCALE, z: type.z * SCALE }, 
              color: { red: type.red, green: type.green, blue: type.blue },
              gravity: {  x: 0, y: GRAVITY, z: 0 },
              velocity: { x: 0, y: 0.05, z: 0 },
            ignoreCollisions: false,
            damping: DAMPING,
            lifetime: LIFETIME,
            dynamic: true })); 
}

function scriptEnding() {
    Entities.editEntity(edge1, { locked: false });
    Entities.editEntity(edge2, { locked: false });
    Entities.editEntity(edge3, { locked: false });
    Entities.editEntity(edge4, { locked: false });
    Entities.editEntity(floor, { locked: false });
    Entities.deleteEntity(edge1);
    Entities.deleteEntity(edge2);
    Entities.deleteEntity(edge3);
    Entities.deleteEntity(edge4);
    Entities.deleteEntity(floor);

    for (var i = 0; i < NUM_BLOCKS; i++) {
        Entities.deleteEntity(blocks[i]);
    }
}

Script.scriptEnding.connect(scriptEnding);
