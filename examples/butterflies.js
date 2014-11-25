//
// butterflies.js
// 
//
// Created by Adrian McCarlie on August 2, 2014
// Modified by Brad Hefta-Gaub to use Entities on Sept. 3, 2014
// Copyright 2014 High Fidelity, Inc.
//
// This sample script creates a swarm of  butterfly entities that fly around your avatar.
//
// Distributed under the Apache License, Version 2.0.
// See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//


var numButterflies = 20;


function getRandomFloat(min, max) {
    return Math.random() * (max - min) + min;
}

// Multiply vector by scalar
function vScalarMult(v, s) {
    var rval = { x: v.x * s, y: v.y * s, z: v.z * s };
    return rval;
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

var NATURAL_SIZE_OF_BUTTERFLY = { x: 1.76, y: 0.825, z: 0.20 };
var lifeTime = 600; // lifetime of the butterflies in seconds
var range = 3.0; // Over what distance in meters do you want the flock to fly around
var frame = 0;

var CHANCE_OF_MOVING = 0.9;
var BUTTERFLY_GRAVITY = 0;
var BUTTERFLY_FLAP_SPEED = 0.5;
var BUTTERFLY_VELOCITY = 0.55;
var DISTANCE_IN_FRONT_OF_ME = 1.5;
var DISTANCE_ABOVE_ME = 1.5;
var flockPosition = Vec3.sum(MyAvatar.position,Vec3.sum(
                        Vec3.multiply(Quat.getFront(MyAvatar.orientation), DISTANCE_ABOVE_ME), 
                        Vec3.multiply(Quat.getFront(MyAvatar.orientation), DISTANCE_IN_FRONT_OF_ME)));
        

// set these pitch, yaw, roll to the needed values to orient the model as you want it
var	pitchInDegrees = 270.0;
var	yawInDegrees = 0.0;
var	rollInDegrees = 0.0;
var	pitchInRadians = pitchInDegrees / 180.0 * Math.PI;
var	yawInRadians = yawInDegrees / 180.0 * Math.PI;
var	rollInRadians = rollInDegrees / 180.0 * Math.PI;

var rotation = Quat.fromPitchYawRollDegrees(pitchInDegrees, yawInDegrees, rollInDegrees);//experimental
	
// This is our butterfly object
function defineButterfly(entityID, targetPosition) {
    this.entityID = entityID;
    this.previousFlapOffset = 0;
    this.targetPosition = targetPosition;
    this.moving = false;
}

// Array of butterflies
var butterflies = [];
function addButterfly() {
    // Decide the size of butterfly 
    var color = { red: 100, green: 100, blue: 100 };
    var size = 0;

    var MINSIZE = 0.06;
    var RANGESIZE = 0.2;
    var maxSize = MINSIZE + RANGESIZE;
    
    size = MINSIZE + Math.random() * RANGESIZE;
    
    var dimensions = Vec3.multiply(NATURAL_SIZE_OF_BUTTERFLY, (size / maxSize));
	
    flockPosition = Vec3.sum(MyAvatar.position,Vec3.sum(
                        Vec3.multiply(Quat.getFront(MyAvatar.orientation), DISTANCE_ABOVE_ME), 
                        Vec3.multiply(Quat.getFront(MyAvatar.orientation), DISTANCE_IN_FRONT_OF_ME)));
	
    var properties = {
        type: "Model",
        lifetime: lifeTime,
        position: Vec3.sum(randVector(-range, range), flockPosition),
        velocity: { x: 0, y: 0.0, z: 0 },
        gravity: { x: 0, y: 1.0, z: 0 },
		damping: 0.1,
		dimensions: dimensions,
        color: color,
		rotation: rotation,
		animationURL: "https://s3-us-west-1.amazonaws.com/highfidelity-public/models/content/butterfly/butterfly.fbx",
		animationIsPlaying: true,
		modelURL: "https://s3-us-west-1.amazonaws.com/highfidelity-public/models/content/butterfly/butterfly.fbx"
    };
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
        Script.stop();
        return;
    }
  
    frame++;
    // Only update every third frame because we don't need to do it too quickly
    if ((frame % 3) == 0) {
        flockPosition = Vec3.sum(MyAvatar.position,Vec3.sum(Vec3.multiply(Quat.getFront(MyAvatar.orientation), DISTANCE_ABOVE_ME), 
                                                            Vec3.multiply(Quat.getFront(MyAvatar.orientation), DISTANCE_IN_FRONT_OF_ME)));
        
        // Update all the butterflies
        for (var i = 0; i < numButterflies; i++) {
            entityID = Entities.identifyEntity(butterflies[i].entityID);
            butterflies[i].entityID = entityID;
            var properties = Entities.getEntityProperties(entityID);
			
    		if (properties.position.y > flockPosition.y + getRandomFloat(0.0,0.3)){ //0.3  //ceiling
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
			
			if (properties.position.y < flockPosition.y - getRandomFloat(0.0,0.3)) { //-0.3   // floor
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
                    var targetPosition = Vec3.sum(randVector(-range, range), flockPosition);
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
		   
				properties.velocity = vInterpolate(properties.velocity, desiredVelocity, 0.5);
				properties.velocity.y = holding ;
			
			
                // If we are near the target, we should get a new target
                var halfLargestDimension = Vec3.length(properties.dimensions) / 2.0;
                if (Vec3.length(Vec3.subtract(properties.position, butterflies[i].targetPosition)) < (halfLargestDimension)) {
                    butterflies[i].moving = false;
                }
				
				var yawRads = Math.atan2(properties.velocity.z, properties.velocity.x); 
				yawRads = yawRads + Math.PI / 2.0;
				var	newOrientation = Quat.fromPitchYawRollRadians(pitchInRadians, yawRads, rollInRadians);
				properties.rotation = newOrientation;
			}

            // Use a cosine wave offset to make it look like its flapping.
            var offset = Math.cos(nowTimeInSeconds * BUTTERFLY_FLAP_SPEED) * (halfLargestDimension);
            properties.position.y = properties.position.y + (offset - butterflies[i].previousFlapOffset);
            // Change position relative to previous offset.
            butterflies[i].previousFlapOffset = offset;
            Entities.editEntity(entityID, properties);
        }
    }
}

// register the call back so it fires before each data send
Script.update.connect(updateButterflies);

//  Delete our little friends if script is stopped
Script.scriptEnding.connect(function() {
    for (var i = 0; i < numButterflies; i++) {
        Entities.deleteEntity(butterflies[i].entityID);
    }
});