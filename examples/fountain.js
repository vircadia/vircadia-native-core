//
//  fountain.js
//  examples
//
//  Copyright 2014 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

function vLength(v) {
    return Math.sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
}

function printVector(v) {
	print(v.x + ", " + v.y + ", " + v.z + "\n");
}

//  Create a random vector with individual lengths between a,b
function randVector(a, b) {
	var rval = { x: a + Math.random() * (b - a), y: a + Math.random() * (b - a), z: a + Math.random() * (b - a) };
	return rval;
}

function vMinus(a, b) { 
	var rval = { x: a.x - b.x, y: a.y - b.y, z: a.z - b.z };
	return rval;
}

function vPlus(a, b) { 
	var rval = { x: a.x + b.x, y: a.y + b.y, z: a.z + b.z };
	return rval;
}

function vCopy(a, b) {
	a.x = b.x;
	a.y = b.y;
	a.z = b.z;
	return;
}

//  Returns a vector which is fraction of the way between a and b
function vInterpolate(a, b, fraction) { 
	var rval = { x: a.x + (b.x - a.x) * fraction, y: a.y + (b.y - a.y) * fraction, z: a.z + (b.z - a.z) * fraction };
	return rval;
}
 
var position = { x: 5.0, y: 0.6, z: 5.0 }; 
Voxels.setVoxel(position.x, 0, position.z, 0.5, 0, 0, 255);
	
var totalEntities = 0;
function makeFountain(deltaTime) {
	if (Math.random() < 0.10) {
	    //print("Made entity!\n");
	    var radius = (0.02 + (Math.random() * 0.05));
        var properties = {
            type: "Sphere",
            position: position, 
            dimensions: { x: radius, y: radius, z: radius}, 
            color: {  red: 0, green: 0, blue: 128 }, 
            velocity: { x: (Math.random() * 1.0 - 0.5),
                         y: (1.0 + (Math.random() * 2.0)),
                         z: (Math.random() * 1.0 - 0.5) }, 
            gravity: {  x: 0, y: -0.1, z: 0 }, 
            damping: 0.25, 
            lifetime: 1
        }

        Entities.addEntity(properties);
        totalEntities++;
    }
    if (totalEntities > 100) {
        Script.stop();
    }
}
// register the call back so it fires before each data send
Script.update.connect(makeFountain);