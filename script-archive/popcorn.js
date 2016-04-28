//
//  popcorn.js
//  examples
//
//  Created by Philip Rosedale on January 25, 2014
//  Copyright 2015 High Fidelity, Inc.
//
//  Creates a bunch of physical balls trapped in a box with a rotating wall in the middle that smacks them around, 
//  and a periodic 'pop' force that shoots them into the air. 
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

var BALL_SIZE = 0.07;
var WALL_THICKNESS = 0.10;
var SCALE = 1.0; 

var GRAVITY = -1.0;
var LIFETIME = 600;
var DAMPING = 0.50;

var TWO_PI = 2.0 * Math.PI;

var center = Vec3.sum(MyAvatar.position, Vec3.multiply(SCALE * 3.0, Quat.getFront(Camera.getOrientation())));

var floor = Entities.addEntity(
            { type: "Box",
            position: Vec3.subtract(center, { x: 0, y: SCALE / 2.0, z: 0 }), 
            dimensions: { x: SCALE, y: WALL_THICKNESS, z: SCALE }, 
              color: { red: 0, green: 255, blue: 0 },
              gravity: {  x: 0, y: 0, z: 0 },
            ignoreCollisions: false,
            lifetime: LIFETIME });

var ceiling = Entities.addEntity(
            { type: "Box",
            position: Vec3.sum(center, { x: 0, y: SCALE / 2.0, z: 0 }), 
            dimensions: { x: SCALE, y: WALL_THICKNESS, z: SCALE }, 
              color: { red: 128, green: 128, blue: 128 },
              gravity: {  x: 0, y: 0, z: 0 },
            ignoreCollisions: false,
            visible: true,
            lifetime: LIFETIME });

var wall1 = Entities.addEntity(
            { type: "Box",
            position: Vec3.sum(center, { x: SCALE / 2.0, y: 0, z: 0 }), 
            dimensions: { x: WALL_THICKNESS, y: SCALE, z: SCALE }, 
              color: { red: 0, green: 255, blue: 0 },
              gravity: {  x: 0, y: 0, z: 0 },
            ignoreCollisions: false,
            visible: false,
            lifetime: LIFETIME });

var wall2 = Entities.addEntity(
            { type: "Box",
            position: Vec3.subtract(center, { x: SCALE / 2.0, y: 0, z: 0 }), 
            dimensions: { x: WALL_THICKNESS, y: SCALE, z: SCALE }, 
              color: { red: 0, green: 255, blue: 0 },
              gravity: {  x: 0, y: 0, z: 0 },
            ignoreCollisions: false,
            visible: false,
            lifetime: LIFETIME });

var wall3 = Entities.addEntity(
            { type: "Box",
            position: Vec3.subtract(center, { x: 0, y: 0, z: SCALE / 2.0 }), 
            dimensions: { x: SCALE, y: SCALE, z: WALL_THICKNESS }, 
              color: { red: 0, green: 255, blue: 0 },
              gravity: {  x: 0, y: 0, z: 0 },
            ignoreCollisions: false,
            visible: false,
            lifetime: LIFETIME }); 

var wall4 = Entities.addEntity(
            { type: "Box",
            position: Vec3.sum(center, { x: 0, y: 0, z: SCALE / 2.0 }), 
            dimensions: { x: SCALE, y: SCALE, z: WALL_THICKNESS }, 
              color: { red: 0, green: 255, blue: 0 },
              gravity: {  x: 0, y: 0, z: 0 },
            ignoreCollisions: false,
            visible: false,
            lifetime: LIFETIME });

var corner1 = Entities.addEntity(
            { type: "Box",
            position: Vec3.sum(center, { x: -SCALE / 2.0, y: 0, z: SCALE / 2.0 }), 
            dimensions: { x: WALL_THICKNESS, y: SCALE, z: WALL_THICKNESS }, 
              color: { red: 128, green: 128, blue: 128 },
            ignoreCollisions: false,
            visible: true,
            lifetime: LIFETIME });

var corner2 = Entities.addEntity(
            { type: "Box",
            position: Vec3.sum(center, { x: -SCALE / 2.0, y: 0, z: -SCALE / 2.0 }), 
            dimensions: { x: WALL_THICKNESS, y: SCALE, z: WALL_THICKNESS }, 
              color: { red: 128, green: 128, blue: 128 },
            ignoreCollisions: false,
            visible: true,
            lifetime: LIFETIME });

var corner3 = Entities.addEntity(
            { type: "Box",
            position: Vec3.sum(center, { x: SCALE / 2.0, y: 0, z: SCALE / 2.0 }), 
            dimensions: { x: WALL_THICKNESS, y: SCALE, z: WALL_THICKNESS }, 
              color: { red: 128, green: 128, blue: 128 },
            ignoreCollisions: false,
            visible: true,
            lifetime: LIFETIME });

var corner4 = Entities.addEntity(
            { type: "Box",
            position: Vec3.sum(center, { x: SCALE / 2.0, y: 0, z: -SCALE / 2.0 }), 
            dimensions: { x: WALL_THICKNESS, y: SCALE, z: WALL_THICKNESS }, 
              color: { red: 128, green: 128, blue: 128 },
            ignoreCollisions: false,
            visible: true,
            lifetime: LIFETIME });

var spinner = Entities.addEntity(
            { type: "Box",
            position: center, 
            dimensions: { x: SCALE / 1.5, y: SCALE / 3.0, z: SCALE / 8.0 }, 
              color: { red: 255, green: 0, blue: 0 },
            // NOTE: angularVelocity is in radians/sec
              angularVelocity: { x: 0, y: TWO_PI, z: 0 },
              angularDamping: 0.0,
              gravity: {  x: 0, y: 0, z: 0 },
            ignoreCollisions: false,
            visible: true,
            lifetime: LIFETIME });

var NUM_BALLS = 70; 

balls = [];

for (var i = 0; i < NUM_BALLS; i++) {
    balls.push(Entities.addEntity(
            { type: "Sphere",
            position: { x: center.x + (Math.random() - 0.5) * (SCALE - BALL_SIZE - WALL_THICKNESS), 
                        y: center.y + (Math.random() - 0.5) * (SCALE - BALL_SIZE - WALL_THICKNESS) , 
                        z: center.z + (Math.random() - 0.5) * (SCALE - BALL_SIZE - WALL_THICKNESS) },  
            dimensions: { x: BALL_SIZE, y: BALL_SIZE, z: BALL_SIZE }, 
              color: { red: Math.random() * 255, green: Math.random() * 255, blue: Math.random() * 255 },
              gravity: {  x: 0, y: GRAVITY, z: 0 },
            ignoreCollisions: false,
            damping: DAMPING,
            lifetime: LIFETIME,
            dynamic: true })); 
}

var VEL_MAG = 2.0;
var CHANCE_OF_POP = 0.007;   //  0.01;
function update(deltaTime) { 
    for (var i = 0; i < NUM_BALLS; i++) {
        if (Math.random() < CHANCE_OF_POP) {
            Entities.editEntity(balls[i], { velocity: { x: (Math.random() - 0.5) * VEL_MAG, y: Math.random() * VEL_MAG, z: (Math.random() - 0.5) * VEL_MAG }});
        }
    }

}
 

function scriptEnding() {
    Entities.deleteEntity(wall1);
    Entities.deleteEntity(wall2);
    Entities.deleteEntity(wall3);
    Entities.deleteEntity(wall4);
    Entities.deleteEntity(corner1);
    Entities.deleteEntity(corner2);
    Entities.deleteEntity(corner3);
    Entities.deleteEntity(corner4);
    Entities.deleteEntity(floor);
    Entities.deleteEntity(ceiling);
    Entities.deleteEntity(spinner);

    for (var i = 0; i < NUM_BALLS; i++) {
        Entities.deleteEntity(balls[i]);
    }
}

Script.scriptEnding.connect(scriptEnding);
Script.update.connect(update);
