//
//  particleBirds.js 
//  examples
//
//  Created by Benjamin Arnold on May 29, 2014
//  Copyright 2014 High Fidelity, Inc.
//
//  This sample script creates a swarm of tweeting bird particles that fly around the avatar.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

// Normalizes a vector to unit length
function vNormalize(v) {
    var length = vLength(v);
    var rval = { x: v.x / length, y: v.y / length, z: v.z / length };
    return rval;
}

// Multiply vector by scalar
function vScalarMult(v, s) {
    var rval = { x: v.x * s, y: v.y * s, z: v.z * s };
    return rval;
}

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

var startTimeInSeconds = new Date().getTime() / 1000;

var birdLifetime = 20; // lifetime of the birds in seconds!
var range = 1.0;       // Over what distance in meters do you want the flock to fly around
var frame = 0;
var moving = false;
var tweeting = 0;

var CHANCE_OF_MOVING = 0.1;
var CHANCE_OF_TWEETING = 0.05;
var BIRD_GRAVITY = -0.1;
var BIRD_FLAP_SPEED = 10.0;
var BIRD_VELOCITY = 0.5;
var myPosition = MyAvatar.position;
var targetPosition = myPosition;

var range = 1.0;   //  Distance around avatar where I can move 

var particleID;
var birdParticleIDs = [];
var birdTweetSounds = [];
var previousFlapOffsets = [];

function addBird()
{
    //  Decide what kind of bird we are 
    var tweet;
    var color;
    var size;
    var which = Math.random();
    if (which < 0.2) {
        tweet = new Sound("https://s3-us-west-1.amazonaws.com/highfidelity-public/sounds/Animals/bushtit_1.raw");
        color = { red: 100, green: 50, blue: 120 };
        size = 0.08;
    } else if (which < 0.4) {
        tweet = new Sound("https://s3-us-west-1.amazonaws.com/highfidelity-public/sounds/Animals/rosyfacedlovebird.raw");
        color = { red: 100, green: 150, blue: 75 };
        size = 0.09;
    } else if (which < 0.6) {
        tweet = new Sound("https://s3-us-west-1.amazonaws.com/highfidelity-public/sounds/Animals/saysphoebe.raw");
        color = { red: 84, green: 121, blue: 36 };
        size = 0.05;
    } else  if (which < 0.8) {
        tweet = new Sound("https://s3-us-west-1.amazonaws.com/highfidelity-public/sounds/Animals/mexicanWhipoorwill.raw");
        color = { red: 23, green: 197, blue: 230 };
        size = 0.12;
    } else {
        tweet = new Sound("https://s3-us-west-1.amazonaws.com/highfidelity-public/sounds/Animals/westernscreechowl.raw");
        color = { red: 50, green: 67, blue: 144 };
        size = 0.15;
    } 
size = 10000;
    var properties = {
        lifetime: birdLifetime,
        position: randVector(-range, range),  
        velocity: { x: 0, y: 0, z: 0 },
        gravity: { x: 0, y: BIRD_GRAVITY, z: 0 },
        radius : size,
        color: color
    };
    
    previousFlapOffsets.push(0.0);
    birdTweetSounds.push(tweet);
    birdParticleIDs.push(Particles.addParticle(properties));
}

var numBirds = 1;

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
            particleID = birdParticleIDs[i];
            var properties = Particles.getParticleProperties(particleID);

            // Tweeting behavior
            if (tweeting == 0) {
                if (Math.random() < CHANCE_OF_TWEETING) {
                    var options = new AudioInjectionOptions();
                    options.position = properties.position;
                    options.volume = 0.75;
                    Audio.playSound(birdTweetSounds[i], options);
                    tweeting = 10;
                }
            } else {
                tweeting -= 1;
            }
         
            // Begin movement by getting a target
            if (moving == false) {
                if (Math.random() < CHANCE_OF_MOVING) {
                    targetPosition = vPlus(randVector(-range, range), myPosition);

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
                    
                    moving = true;
                }
            }
            // If we are moving, move towards the target
            if (moving) {
                var desiredVelocity = vMinus(targetPosition, properties.position);
                desiredVelocity = vScalarMult(vNormalize(desiredVelocity), BIRD_VELOCITY);
                    
                properties.velocity = vInterpolate(properties.velocity, desiredVelocity, 0.2);
                // If we are near the target, we should get a new target
                if (vLength(vMinus(properties.position, targetPosition)) < (properties.radius / 5.0)) {
                    moving = false;
                }
            }
            
            // Use a cosine wave offset to make it look like its flapping. 
            var offset = Math.cos(nowTimeInSeconds * BIRD_FLAP_SPEED) * properties.radius;
            properties.position.y = properties.position.y + (offset - previousFlapOffsets[i]);
            // Change position relative to previous offset.
            previousFlapOffsets[i] = offset;
            
            // Update the particle
            Particles.editParticle(particleID, properties);
        }
    }
}

// register the call back so it fires before each data send
Script.update.connect(updateBirds);
