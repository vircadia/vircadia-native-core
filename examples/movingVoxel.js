//
//  movingVoxel.js
//  examples
//
//  Copyright 2014 High Fidelity, Inc.
//
//  This sample script creates a voxel moving back and forth in a line
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

var position  = { x: 0, y: 0, z: 0 };
var oldPosition = { x: 0, y: 0, z:0 };
var size = 0.25;
var direction = 1.0;
var range = 2.0;
var color = { r: 100, g: 50, b: 150 };
var colorEdge = { r:255, g:250, b:175 };
var frame = 0;
var thisColor = color;

function moveVoxel(deltaTime) {
    frame++;
	if (frame % 3 == 0) {
		//  Get a new position
		position.x += direction * size;
		if (position.x < 0) {
			direction *= -1.0;
			position.x = 0;
            thisColor = colorEdge;
		}
		if (position.x > range) { 
			direction *= -1.0;
			position.x = range;
            thisColor = colorEdge;
		}
		//  Create a new voxel
		Voxels.setVoxel(position.x, position.y, position.z, size, thisColor.r, thisColor.g, thisColor.b);
		//  delete old voxel 
		Voxels.eraseVoxel(oldPosition.x, oldPosition.y, oldPosition.z, size);
		//  Copy old location to new 
		oldPosition.x = position.x;
        oldPosition.y = position.y;
        oldPosition.z = position.z;
        thisColor = color;
	}
}

Voxels.setPacketsPerSecond(300);
// Connect a call back that happens every frame
Script.update.connect(moveVoxel);