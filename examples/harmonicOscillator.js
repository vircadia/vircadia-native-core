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
var speed = 0.5;


var basePosition = Vec3.sum(Camera.getPosition(), Quat.getFront(Camera.getOrientation()));

ball = Entities.addEntity(
  { type: "Box",
    position: basePosition,
	dimensions: { x: 0.1, y: 0.1, z: 0.1 },  
    color: { red: 255, green: 0, blue: 255 }
  });

disc = Entities.addEntity(
  { type: "Sphere",
    position: basePosition,
	dimensions: { x: range, y: range / 20.0, z: range },  
    color: { red: 128, green: 128, blue: 128 }
  });

function update(deltaTime) {
  time += deltaTime * speed;
  if (!ball.isKnownID) {
     ball = Entities.identifyEntity(ball);
  }	 
  rotation = Quat.angleAxis(time/Math.PI * 180.0, { x: 0, y: 1, z: 0 });
  Entities.editEntity(ball, 
	{
	  color: { red: 255 * (Math.sin(time)/2.0 + 0.5), 
               green: 255 - 255 * (Math.sin(time)/2.0 + 0.5), 
               blue: 0 },
	  position: { x: basePosition.x + Math.sin(time) / 2.0 * range, 
                  y: basePosition.y, 
				 z: basePosition.z + Math.cos(time) / 2.0 * range },  
	  velocity: { x: Math.cos(time)/2.0 * range, 
                  y: 0.0, 
				 z: -Math.sin(time)/2.0 * range },
	  rotation: rotation 
    }); 
}

function scriptEnding() {
    Entities.deleteEntity(ball);
	Entities.deleteEntity(disc);
}

Script.scriptEnding.connect(scriptEnding);
Script.update.connect(update);
