// harmonicOscillator.js 
// 
// Created by Philip Rosedale on May 5, 2015 
// Copyright 2015 High Fidelity, Inc.
//
// An object moves around the edge of a disc while 
// changing color.  The script is continuously updating
// position, velocity, rotation, and color.  The movement 
// should appear perfectly smooth to someone else, 
// provided their network connection is good.  
//
// Distributed under the Apache License, Version 2.0.
// See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

var ball, disc; 
var time = 0.0; 
var range = 1.0;
var omega = 2.0 * Math.PI / 32.0;

var basePosition = Vec3.sum(Camera.getPosition(), Quat.getFront(Camera.getOrientation()));

ball = Entities.addEntity(
  { type: "Box",
    position: basePosition,
    dimensions: { x: 0.1, y: 0.1, z: 0.1 },  
    color: { red: 255, green: 0, blue: 255 },
    dynamic: false,
    collisionless: true
  });

disc = Entities.addEntity(
  { type: "Sphere",
    position: basePosition,
    dimensions: { x: range * 0.8, y: range / 20.0, z: range * 0.8},  
    color: { red: 128, green: 128, blue: 128 }
  });

function randomColor() {
  return { red: Math.random() * 255, green: Math.random() * 255, blue: Math.random() * 255 };
}

function update(deltaTime) {
  time += deltaTime;
  rotation = Quat.angleAxis(time * omega  /Math.PI * 180.0, { x: 0, y: 1, z: 0 });
  Entities.editEntity(ball, 
    {
      color: { red: 255 * (Math.sin(time * omega)/2.0 + 0.5), 
               green: 255 - 255 * (Math.sin(time * omega)/2.0 + 0.5), 
               blue: 0 },
      position: { x: basePosition.x + Math.sin(time * omega) / 2.0 * range, 
                  y: basePosition.y, 
                 z: basePosition.z + Math.cos(time * omega) / 2.0 * range },  
      velocity: { x: Math.cos(time * omega)/2.0 * range * omega, 
                  y: 0.0, 
                 z: -Math.sin(time * omega)/2.0 * range * omega},
      rotation: rotation 
    }); 
  if (Math.random() < 0.007) {
    Entities.editEntity(disc, { color: randomColor() });
  }
}

function scriptEnding() {
    Entities.deleteEntity(ball);
    Entities.deleteEntity(disc);
}

Script.scriptEnding.connect(scriptEnding);
Script.update.connect(update);
