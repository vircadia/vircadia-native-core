//
// butterflyFlockTest1.js
// 
//
// Created by Adrian McCarlie on August 2, 2014
// Modified by Brad Hefta-Gaub to use Entities on Sept. 3, 2014
// Copyright 2014 High Fidelity, Inc.
//
// This sample script creates a swarm of  butterfly entities that fly around the avatar.
//
// Distributed under the Apache License, Version 2.0.
// See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//


function getRandomFloat(min, max) {
    return Math.random() * (max - min) + min;
}

// Multiply vector by scalar
function vScalarMult(v, s) {
    var rval = { x: v.x * s, y: v.y * s, z: v.z * s };
    return rval;
}

function printVector(v) {
    print(v.x + ", " + v.y + ", " + v.z + "\n");
}
// Create a random vector with individual lengths between a,b
function randVector(a, b) {
    var rval = { x: a + Math.random() * (b - a), y: a + Math.random() * (b - a), z: a + Math.random() * (b - a) };
    return rval;
}

// Returns a vector which is fraction of the way between a and b
function vInterpolate(a, b, fraction) {
    var rval = { x: a.x + (b.x - a.x) * fraction, y: a.y + (b.y - a.y) * fraction, z: a.z + (b.z - a.z) * fraction };
    return rval;
}

var startTimeInSeconds = new Date().getTime() / 1000;

var lifeTime = 60; // lifetime of the butterflies in seconds!
var range = 1.0; // Over what distance in meters do you want the flock to fly around
var frame = 0;

var CHANCE_OF_MOVING = 0.9;
var BUTTERFLY_GRAVITY = 0;//-0.06;
var BUTTERFLY_FLAP_SPEED = 1.0;
var BUTTERFLY_VELOCITY = 0.55;
var myPosition = MyAvatar.position;

var	pitch = 0.0;//experimental
var	yaw = 0.0;//experimental
var	roll = 0.0;	//experimental
var rotation = Quat.fromPitchYawRollDegrees(pitch, yaw, roll);//experimental
	
// This is our butterfly object
function defineButterfly(entityID, targetPosition) {
    this.entityID = entityID;
    this.previousFlapOffset = 0;
    this.targetPosition = targetPosition;
    this.moving = false;
}

// Array of butterflies
var butterflies = [];
var numButterflies = 20;
function addButterfly() {
    // Decide the size of butterfly 
    var color = { red: 100, green: 100, blue: 100 };
    var size = 0;

    var which = Math.random();
    if (which < 0.2) {
        size = 0.08;
    } else if (which < 0.4) {
        size = 0.09;
    } else if (which < 0.6) {
        size = 0.8;
    } else if (which < 0.8) {
        size = 0.8;
    } else {
        size = 0.8;
    }
	
	myPosition = MyAvatar.position;	
    //	if ( frame < numButterflies){
    //	 myPosition = {x: myPosition.x, y: myPosition.y, z: myPosition.z };
    //	}
	
    var properties = {
        type: "Model",
        lifetime: lifeTime,
        position: Vec3.sum(randVector(-range, range), myPosition),
        velocity: { x: 0, y: 0.0, z: 0 },
        gravity: { x: 0, y: 1.0, z: 0 },
		damping: 0.1,
        radius : size,
        color: color,
		rotation: rotation,
		//animationURL: "http://business.ozblog.me/objects/butterfly/newButterfly6.fbx",
		//animationIsPlaying: true,
		modelURL: "http://business.ozblog.me/objects/butterfly/newButterfly6.fbx"
    };
    properties.position.z = properties.position.z+1;
    butterflies.push(new defineButterfly(Entities.addEntity(properties), properties.position));
}

// Generate the butterflies
for (var i = 0; i < numButterflies; i++) {
    addButterfly();
}

// Main update function
function updateButterflies(deltaTime) {
    // Check to see if we've been running long enough that our butterflies are dead
    var nowTimeInSeconds = new Date().getTime() / 1000;
    if ((nowTimeInSeconds - startTimeInSeconds) >= lifeTime) {
        //  print("our butterflies are dying, stop our script");
        Script.stop();
        return;
    }
  
    frame++;
    // Only update every third frame
    if ((frame % 3) == 0) {
        myPosition = MyAvatar.position;
        
        // Update all the butterflies
        for (var i = 0; i < numButterflies; i++) {
            entityID = butterflies[i].entityID;
            var properties = Entities.getEntityProperties(entityID);
			
    		if (properties.position.y > myPosition.y + getRandomFloat(0.0,0.3)){ //0.3  //ceiling
                properties.gravity.y = - 3.0;
                properties.damping.y = 1.0;
                properties.velocity.y = 0;
                properties.velocity.x = properties.velocity.x;
                properties.velocity.z = properties.velocity.z;	
                if (properties.velocity.x < 0.5){ 
                    butterflies[i].moving = false;
                }
                if (properties.velocity.z < 0.5){ 
                    butterflies[i].moving = false;
                }				
			}
			
			if (properties.velocity.y <= -0.2) {
                properties.velocity.y = 0.22;		
                properties.velocity.x = properties.velocity.x;
                properties.velocity.z = properties.velocity.z;
			}
			
			if (properties.position.y < myPosition.y - getRandomFloat(0.0,0.3)) { //-0.3   // floor
                properties.velocity.y = 0.9;
                properties.gravity.y = - 4.0;
                properties.velocity.x = properties.velocity.x;
                properties.velocity.z = properties.velocity.z;
                if (properties.velocity.x < 0.5){ 
                    butterflies[i].moving = false;
				}
                if (properties.velocity.z < 0.5){ 
                    butterflies[i].moving = false;
				}			
			}


            // Begin movement by getting a target
            if (butterflies[i].moving == false) {
                if (Math.random() < CHANCE_OF_MOVING) {
                    var targetPosition = Vec3.sum(randVector(-range, range), myPosition);
                    if (targetPosition.x < 0) {
                        targetPosition.x = 0;
                    }
                    if (targetPosition.y < 0) {
                        targetPosition.y = 0;
                    }
                    if (targetPosition.z < 0) {
                        targetPosition.z = 0;
                    }
                    if (targetPosition.x > TREE_SCALE) {
                        targetPosition.x = TREE_SCALE;
                    }
                    if (targetPosition.y > TREE_SCALE) {
                        targetPosition.y = TREE_SCALE;
                    }
                    if (targetPosition.z > TREE_SCALE) {
                        targetPosition.z = TREE_SCALE;
                    }
                    butterflies[i].targetPosition = targetPosition;
                    butterflies[i].moving = true;
                }
            }
			  
            // If we are moving, move towards the target
            if (butterflies[i].moving) {	
			
				var	holding = properties.velocity.y;
			
                var desiredVelocity = Vec3.subtract(butterflies[i].targetPosition, properties.position);
                desiredVelocity = vScalarMult(Vec3.normalize(desiredVelocity), BUTTERFLY_VELOCITY);
		   
				properties.velocity = vInterpolate(properties.velocity, desiredVelocity, 0.2);
				properties.velocity.y = holding ;
			
			
                // If we are near the target, we should get a new target
                if (Vec3.length(Vec3.subtract(properties.position, butterflies[i].targetPosition)) < (properties.radius / 1.0)) {
                    butterflies[i].moving = false;
                }
				
				var yawRads = Math.atan2(properties.velocity.z, properties.velocity.x); 
				yawRads = yawRads + Math.PI / 2.0;
				var	newOrientation = Quat.fromPitchYawRollRadians(0.0, yawRads, 0.0);
				properties.rotation = newOrientation;
			}

            // Use a cosine wave offset to make it look like its flapping.
            var offset = Math.cos(nowTimeInSeconds * BUTTERFLY_FLAP_SPEED) * (properties.radius);
            properties.position.y = properties.position.y + (offset - butterflies[i].previousFlapOffset);
            // Change position relative to previous offset.
            butterflies[i].previousFlapOffset = offset;
            Entities.editEntity(entityID, properties);
        }
    }
}

// register the call back so it fires before each data send
Script.update.connect(updateButterflies);