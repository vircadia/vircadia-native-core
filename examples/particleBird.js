//
//  particleBird.js 
//  examples
//
//  Copyright 2014 High Fidelity, Inc.
//
//  This sample script moves a voxel around like a bird and sometimes makes tweeting noises 
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

//  Decide what kind of bird we are 
var tweet;
var color;
var size;
var which = Math.random();
if (which < 0.2) {
    tweet = new Sound("https://s3-us-west-1.amazonaws.com/highfidelity-public/sounds/Animals/bushtit_1.raw");
    color = { r: 100, g: 50, b: 120 };
    size = 0.08;
} else if (which < 0.4) {
    tweet = new Sound("https://s3-us-west-1.amazonaws.com/highfidelity-public/sounds/Animals/rosyfacedlovebird.raw");
    color = { r: 100, g: 150, b: 75 };
    size = 0.09;
} else if (which < 0.6) {
    tweet = new Sound("https://s3-us-west-1.amazonaws.com/highfidelity-public/sounds/Animals/saysphoebe.raw");
    color = { r: 84, g: 121, b: 36 };
    size = 0.05;
} else  if (which < 0.8) {
    tweet = new Sound("https://s3-us-west-1.amazonaws.com/highfidelity-public/sounds/Animals/mexicanWhipoorwill.raw");
    color = { r: 23, g: 197, b: 230 };
    size = 0.12;
} else {
    tweet = new Sound("https://s3-us-west-1.amazonaws.com/highfidelity-public/sounds/Animals/westernscreechowl.raw");
    color = { r: 50, g: 67, b: 144 };
    size = 0.15;
} 


var startTimeInSeconds = new Date().getTime() / 1000;

var birdLifetime = 20; // lifetime of the bird in seconds!
var position  = { x: 0, y: 0, z: 0 };
var targetPosition = { x: 0, y: 0, z: 0 };
var range = 1.0;       //   Over what distance in meters do you want your bird to fly around 
var frame = 0;
var moving = false;
var tweeting = 0;
var moved = false;

var CHANCE_OF_MOVING = 0.00;
var CHANCE_OF_FLAPPING = 0.05;
var CHANCE_OF_TWEETING = 0.05;
var START_HEIGHT_ABOVE_ME = 1.5;
var BIRD_GRAVITY = -0.1;
var BIRD_FLAP = 1.0;
var myPosition = MyAvatar.position;
var properties = {
    lifetime: birdLifetime,
    position: { x: myPosition.x, y: myPosition.y + START_HEIGHT_ABOVE_ME, z: myPosition.z },  
    velocity: { x: 0, y: Math.random() * BIRD_FLAP, z: 0 },
    gravity: { x: 0, y: BIRD_GRAVITY, z: 0 },
    radius : 0.1,
    color: { red: 0,
         green: 255,
         blue: 0 }
};
var range = 1.0;   //  Distance around avatar where I can move 
//  Create the actual bird 
var particleID = Particles.addParticle(properties);
function moveBird(deltaTime) {

    // check to see if we've been running long enough that our bird is dead
    var nowTimeInSeconds = new Date().getTime() / 1000;
    if ((nowTimeInSeconds - startTimeInSeconds) >= birdLifetime) {

        print("our bird is dying, stop our script");
        Script.stop();
        return;
    }

    myPosition = MyAvatar.position;
    frame++;
    if (frame % 3 == 0) {
        // Tweeting behavior
        if (tweeting == 0) {
            if (Math.random() < CHANCE_OF_TWEETING) {
                //print("tweet!" + "\n");
                var options = new AudioInjectionOptions();
                options.position = position;
                options.volume = 0.75;
                Audio.playSound(tweet, options);
                tweeting = 10;
            }
        } else {
            tweeting -= 1;
        }
        if (Math.random() < CHANCE_OF_FLAPPING) { 
            //  Add a little upward impulse to our bird 
            // TODO:  Get velocity 
            // 
            var newProperties = {
                    velocity: { x:0.0, y: Math.random() * BIRD_FLAP, z: 0.0 }
                };
                Particles.editParticle(particleID, newProperties);
                print("flap!");
        }
        // Moving behavior 
        if (moving == false) {
            if (Math.random() < CHANCE_OF_MOVING) {
                targetPosition = randVector(-range, range);
                targetPosition = vPlus(targetPosition, myPosition);

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
                //printVector(position);
                moving = true;
            }
        }
        if (moving) {
            position = vInterpolate(position, targetPosition, 0.5);
            if (vLength(vMinus(position, targetPosition)) < (size / 5.0)) {
                moved = false;
                moving = false;
            } else {
                moved = true;
            }
        }
        if (moved || (tweeting > 0)) {
            if (tweeting > 0) {
                var newProperties = {
                    position: position,
                    radius : size * 1.5,
                    color: { red: Math.random() * 255, green: 0, blue: 0 }
                };
            } else {
                var newProperties = {
                    position: position,
                    radius : size,
                    color: { red: color.r, green: color.g, blue: color.b }
                    };
            }
            Particles.editParticle(particleID, newProperties);
            moved = false;
        }
    }
}

// register the call back so it fires before each data send
Script.update.connect(moveBird);
