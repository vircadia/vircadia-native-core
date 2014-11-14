//
//  particleBirds.js 
//  examples
//
//  Created by Benjamin Arnold on May 29, 2014
//  Copyright 2014 High Fidelity, Inc.
//
//  This sample script creates a swarm of tweeting bird entities that fly around the avatar.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

Script.include("libraries/globals.js");

// Multiply vector by scalar
function vScalarMult(v, s) {
    var rval = { x: v.x * s, y: v.y * s, z: v.z * s };
    return rval;
}

function printVector(v) {
    print(v.x + ", " + v.y + ", " + v.z + "\n");
}
//  Create a random vector with individual lengths between a,b
function randVector(a, b) {
    var rval = { x: a + Math.random() * (b - a), y: a + Math.random() * (b - a), z: a + Math.random() * (b - a) };
    return rval;
}

//  Returns a vector which is fraction of the way between a and b
function vInterpolate(a, b, fraction) { 
    var rval = { x: a.x + (b.x - a.x) * fraction, y: a.y + (b.y - a.y) * fraction, z: a.z + (b.z - a.z) * fraction };
    return rval;
}

var startTimeInSeconds = new Date().getTime() / 1000;

var birdLifetime = 20; // lifetime of the birds in seconds!
var range = 1.0;       // Over what distance in meters do you want the flock to fly around
var frame = 0;

var CHANCE_OF_MOVING = 0.1;
var CHANCE_OF_TWEETING = 0.05;
var BIRD_GRAVITY = -0.1;
var BIRD_FLAP_SPEED = 10.0;
var BIRD_VELOCITY = 0.5;
var myPosition = MyAvatar.position;

var range = 1.0;   //  Distance around avatar where I can move 

// This is our Bird object
function Bird (particleID, tweetSound, targetPosition) {
    this.particleID = particleID;
    this.tweetSound = tweetSound;
    this.previousFlapOffset = 0;
    this.targetPosition = targetPosition;
    this.moving = false;
    this.tweeting = -1;
}

// Array of birds
var birds = [];

function addBird()
{
    //  Decide what kind of bird we are 
    var tweet;
    var color;
    var size;
    var which = Math.random();
    if (which < 0.2) {
        tweet = SoundCache.getSound(HIFI_PUBLIC_BUCKET + "sounds/Animals/bushtit_1.raw");
        color = { red: 100, green: 50, blue: 120 };
        size = 0.08;
    } else if (which < 0.4) {
        tweet = SoundCache.getSound(HIFI_PUBLIC_BUCKET + "sounds/Animals/rosyfacedlovebird.raw");
        color = { red: 100, green: 150, blue: 75 };
        size = 0.09;
    } else if (which < 0.6) {
        tweet = SoundCache.getSound(HIFI_PUBLIC_BUCKET + "sounds/Animals/saysphoebe.raw");
        color = { red: 84, green: 121, blue: 36 };
        size = 0.05;
    } else  if (which < 0.8) {
        tweet = SoundCache.getSound(HIFI_PUBLIC_BUCKET + "sounds/Animals/mexicanWhipoorwill.raw");
        color = { red: 23, green: 197, blue: 230 };
        size = 0.12;
    } else {
        tweet = SoundCache.getSound(HIFI_PUBLIC_BUCKET + "sounds/Animals/westernscreechowl.raw");
        color = { red: 50, green: 67, blue: 144 };
        size = 0.15;
    } 
    var properties = {
        type: "Sphere",
        lifetime: birdLifetime,
        position: Vec3.sum(randVector(-range, range), myPosition),  
        velocity: { x: 0, y: 0, z: 0 },
        gravity: { x: 0, y: BIRD_GRAVITY, z: 0 },
        dimensions: { x: size * 2, y: size * 2, z: size * 2 },
        color: color
    };
    
    birds.push(new Bird(Entities.addEntity(properties), tweet, properties.position));
}

var numBirds = 30;

// Generate the birds
for (var i = 0; i < numBirds; i++) {
    addBird();
}

// Main update function
function updateBirds(deltaTime) {

    // Check to see if we've been running long enough that our birds are dead
    var nowTimeInSeconds = new Date().getTime() / 1000;
    if ((nowTimeInSeconds - startTimeInSeconds) >= birdLifetime) {

        print("our birds are dying, stop our script");
        Script.stop();
        return;
    }

    frame++;
    // Only update every third frame
    if ((frame % 3) == 0) {
        myPosition = MyAvatar.position;
        
        // Update all the birds
        for (var i = 0; i < numBirds; i++) {
            particleID = birds[i].particleID;
            var properties = Entities.getEntityProperties(particleID);

            // Tweeting behavior
            if (birds[i].tweeting == 0) {
                if (Math.random() < CHANCE_OF_TWEETING) {
                    Audio.playSound(birds[i].tweetSound, {
                      position: properties.position,
                      volume: 0.75
                    });
                    birds[i].tweeting = 10;
                }
            } else {
                birds[i].tweeting -= 1;
            }
         
            // Begin movement by getting a target
            if (birds[i].moving == false) {
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
                    
                    birds[i].targetPosition = targetPosition;
                    
                    birds[i].moving = true;
                }
            }
            // If we are moving, move towards the target
            if (birds[i].moving) {
                var desiredVelocity = Vec3.subtract(birds[i].targetPosition, properties.position);
                desiredVelocity = vScalarMult(Vec3.normalize(desiredVelocity), BIRD_VELOCITY);
                    
                properties.velocity = vInterpolate(properties.velocity, desiredVelocity, 0.2);
                // If we are near the target, we should get a new target
                if (Vec3.length(Vec3.subtract(properties.position, birds[i].targetPosition)) < (properties.dimensions.x / 5.0)) {
                    birds[i].moving = false;
                }
            }
            
            // Use a cosine wave offset to make it look like its flapping. 
            var offset = Math.cos(nowTimeInSeconds * BIRD_FLAP_SPEED) * properties.dimensions.x;
            properties.position.y = properties.position.y + (offset - birds[i].previousFlapOffset);
            // Change position relative to previous offset.
            birds[i].previousFlapOffset = offset;
            
            // Update the particle
            Entities.editEntity(particleID, properties);
        }
    }
}

// register the call back so it fires before each data send
Script.update.connect(updateBirds);
