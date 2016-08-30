//
//  flockOfbirds.js
//  examples
//
//  Copyright 2014 High Fidelity, Inc.
//  Creates a flock of birds that fly around and chirp, staying inside the corners of the box defined
//  at the start of the script.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html


//  The rectangular area in the domain where the flock will fly
var lowerCorner = { x: 0, y: 0, z: 0 };
var upperCorner = { x: 30, y: 10, z: 30  };
var STARTING_FRACTION = 0.25;

var NUM_BIRDS = 50;
var UPDATE_INTERVAL = 0.016;
var playSounds = true;
var SOUND_PROBABILITY = 0.001;
var STARTING_LIFETIME = (1.0 / SOUND_PROBABILITY) * UPDATE_INTERVAL * 10;
var numPlaying = 0;
var BIRD_SIZE = 0.08;
var BIRD_MASTER_VOLUME = 0.1;
var FLAP_PROBABILITY = 0.005;
var RANDOM_FLAP_VELOCITY = 1.0;
var FLAP_UP = 1.0;
var BIRD_GRAVITY = -0.5;
var LINEAR_DAMPING = 0.2;
var FLAP_FALLING_PROBABILITY = 0.025;
var MIN_ALIGNMENT_VELOCITY = 0.0;
var MAX_ALIGNMENT_VELOCITY = 1.0;
var VERTICAL_ALIGNMENT_COUPLING = 0.0;
var ALIGNMENT_FORCE = 1.5;
var COHESION_FORCE = 1.0;
var MAX_COHESION_VELOCITY = 0.5;

var followBirds = false;
var AVATAR_FOLLOW_RATE = 0.001;
var AVATAR_FOLLOW_VELOCITY_TIMESCALE = 2.0;
var AVATAR_FOLLOW_ORIENTATION_RATE = 0.005;
var floor = false;
var MAKE_FLOOR = false;

var averageVelocity = { x: 0, y: 0, z: 0 };
var averagePosition = { x: 0, y: 0, z: 0 };

var birdsLoaded = false;

var oldAvatarOrientation;
var oldAvatarPosition;

var birds = [];
var playing = [];

function randomVector(scale) {
    return { x: Math.random() * scale - scale / 2.0, y: Math.random() * scale - scale / 2.0, z: Math.random() * scale - scale / 2.0 };
}

function updateBirds(deltaTime) {
    if (!Entities.serversExist() || !Entities.canRez()) {
        return;
    }
    if (!birdsLoaded) {
        loadBirds(NUM_BIRDS);
        birdsLoaded = true;
        return;
    }
    var sumVelocity = { x: 0, y: 0, z: 0 };
    var sumPosition = { x: 0, y: 0, z: 0 };
    var birdPositionsCounted = 0;
    var birdVelocitiesCounted = 0;
    for (var i = 0; i < birds.length; i++) {
        if (birds[i].entityId) {
            var properties = Entities.getEntityProperties(birds[i].entityId);
            //  If Bird has been deleted, bail
            if (properties.id != birds[i].entityId) {
                birds[i].entityId = false;
                return;
            }
            //  Sum up average position and velocity
            if (Vec3.length(properties.velocity) > MIN_ALIGNMENT_VELOCITY) {
                sumVelocity = Vec3.sum(sumVelocity, properties.velocity);
                birdVelocitiesCounted += 1;
            }
            sumPosition = Vec3.sum(sumPosition, properties.position);
            birdPositionsCounted += 1;

            var downwardSpeed = (properties.velocity.y < 0) ? -properties.velocity.y : 0.0;
            if ((properties.position.y < upperCorner.y) && (Math.random() < (FLAP_PROBABILITY + (downwardSpeed * FLAP_FALLING_PROBABILITY)))) {
                //  More likely to flap if falling
                var randomVelocity = randomVector(RANDOM_FLAP_VELOCITY);
                randomVelocity.y = FLAP_UP + Math.random() * FLAP_UP;

                //  Alignment Velocity
                var alignmentVelocityMagnitude = Math.min(MAX_ALIGNMENT_VELOCITY, Vec3.length(Vec3.multiply(ALIGNMENT_FORCE, averageVelocity)));
                var alignmentVelocity = Vec3.multiply(alignmentVelocityMagnitude, Vec3.normalize(averageVelocity));
                alignmentVelocity.y *= VERTICAL_ALIGNMENT_COUPLING;

                //  Cohesion
                var distanceFromCenter = Vec3.length(Vec3.subtract(averagePosition, properties.position));
                var cohesionVelocitySize = Math.min(distanceFromCenter * COHESION_FORCE, MAX_COHESION_VELOCITY);
                var cohesionVelocity = Vec3.multiply(cohesionVelocitySize, Vec3.normalize(Vec3.subtract(averagePosition, properties.position)));

                var newVelocity = Vec3.sum(randomVelocity, Vec3.sum(alignmentVelocity, cohesionVelocity));

                Entities.editEntity(birds[i].entityId, { velocity: Vec3.sum(properties.velocity, newVelocity) });

            }

            //  Check whether to play a chirp
            if (playSounds && (!birds[i].audioId || !birds[i].audioId.playing) && (Math.random() < ((numPlaying > 0) ? SOUND_PROBABILITY / numPlaying : SOUND_PROBABILITY))) {
                var options = {
                          position: properties.position,
                    volume: BIRD_MASTER_VOLUME
                };
                // Play chirp
                if (birds[i].audioId) {
                    birds[i].audioId.setOptions(options);
                    birds[i].audioId.restart();
                } else {
                    birds[i].audioId = Audio.playSound(birds[i].sound, options);
                }
                numPlaying++;
                // Change size, and update lifetime to keep bird alive
                Entities.editEntity(birds[i].entityId, { dimensions: Vec3.multiply(1.5, properties.dimensions),
                                                         lifetime: properties.ageInSeconds + STARTING_LIFETIME});

            } else if (birds[i].audioId) {
                // If bird is playing a chirp
                if (!birds[i].audioId.playing) {
                    Entities.editEntity(birds[i].entityId, { dimensions: { x: BIRD_SIZE, y: BIRD_SIZE, z: BIRD_SIZE }});
                    numPlaying--;
                }
            }

            //  Keep birds in their 'cage'
            var bounce = false;
            var newVelocity = properties.velocity;
            var newPosition = properties.position;
            if (properties.position.x < lowerCorner.x) {
                newPosition.x = lowerCorner.x;
                newVelocity.x *= -1.0;
                bounce = true;
            } else if (properties.position.x > upperCorner.x) {
                newPosition.x = upperCorner.x;
                newVelocity.x *= -1.0;
                bounce = true;
            }
            if (properties.position.y < lowerCorner.y) {
                newPosition.y = lowerCorner.y;
                newVelocity.y *= -1.0;
                bounce = true;
            } else if (properties.position.y > upperCorner.y) {
                newPosition.y = upperCorner.y;
                newVelocity.y *= -1.0;
                bounce = true;
            }
            if (properties.position.z < lowerCorner.z) {
                newPosition.z = lowerCorner.z;
                newVelocity.z *= -1.0;
                bounce = true;
            } else if (properties.position.z > upperCorner.z) {
                newPosition.z = upperCorner.z;
                newVelocity.z *= -1.0;
                bounce = true;
            }
            if (bounce) {
                Entities.editEntity(birds[i].entityId, { position: newPosition, velocity: newVelocity });
            }
        }
    }
    //  Update average velocity and position of flock
    if (birdVelocitiesCounted > 0) {
        averageVelocity = Vec3.multiply(1.0 / birdVelocitiesCounted, sumVelocity);
        //print(Vec3.length(averageVelocity));
        if (followBirds) {
            MyAvatar.motorVelocity = averageVelocity;
            MyAvatar.motorTimescale = AVATAR_FOLLOW_VELOCITY_TIMESCALE;
            var polarAngles = Vec3.toPolar(Vec3.normalize(averageVelocity));
            if (!isNaN(polarAngles.x) && !isNaN(polarAngles.y)) {
                var birdDirection = Quat.fromPitchYawRollRadians(polarAngles.x, polarAngles.y + Math.PI, polarAngles.z);
                MyAvatar.orientation = Quat.mix(MyAvatar.orientation, birdDirection, AVATAR_FOLLOW_ORIENTATION_RATE);
            }
        }
    }
    if (birdPositionsCounted > 0) {
        averagePosition = Vec3.multiply(1.0 / birdPositionsCounted, sumPosition);
            //  If Following birds, update position
        if (followBirds) {
            MyAvatar.position = Vec3.sum(Vec3.multiply(AVATAR_FOLLOW_RATE, MyAvatar.position), Vec3.multiply(1.0 - AVATAR_FOLLOW_RATE, averagePosition));
        }
    }

}

// Connect a call back that happens every frame
Script.update.connect(updateBirds);

//  Delete our little friends if script is stopped
Script.scriptEnding.connect(function() {
    for (var i = 0; i < birds.length; i++) {
        Entities.deleteEntity(birds[i].entityId);
    }
    if (floor) {
        Entities.deleteEntity(floor);
    }
    MyAvatar.orientation = oldAvatarOrientation;
    MyAvatar.position = oldAvatarPosition;
});

function loadBirds(howMany) {
    oldAvatarOrientation = MyAvatar.orientation;
    oldAvatarPosition = MyAvatar.position;

  var sound_filenames = ["bushtit_1.raw", "bushtit_2.raw", "bushtit_3.raw"];
  /* Here are more sounds/species you can use
                        , "mexicanWhipoorwill.raw",
                           "rosyfacedlovebird.raw", "saysphoebe.raw", "westernscreechowl.raw", "bandtailedpigeon.wav", "bridledtitmouse.wav",
                           "browncrestedflycatcher.wav", "commonnighthawk.wav", "commonpoorwill.wav", "doublecrestedcormorant.wav",
                           "gambelsquail.wav", "goldcrownedkinglet.wav", "greaterroadrunner.wav","groovebilledani.wav","hairywoodpecker.wav",
                           "housewren.wav","hummingbird.wav", "mountainchickadee.wav", "nightjar.wav", "piebilledgrieb.wav", "pygmynuthatch.wav",
                           "whistlingduck.wav", "woodpecker.wav"];
                         */

  var colors = [
              { red: 242, green: 207, blue: 013 },
            { red: 238, green: 94, blue: 11 },
            { red: 81, green: 30, blue: 7 },
            { red: 195, green: 176, blue: 81 },
            { red: 235, green: 190, blue: 152 },
            { red: 167, green: 99, blue: 52 },
            { red: 199, green: 122, blue: 108 },
            { red: 246, green: 220, blue: 189 },
            { red: 208, green: 145, blue: 65 },
            { red: 173, green: 120 , blue: 71 },
            { red: 132, green: 147, blue: 174 },
            { red: 164, green: 74, blue: 40 },
            { red: 131, green: 127, blue: 134 },
            { red: 209, green: 157, blue: 117 },
            { red: 205, green: 191, blue: 193 },
            { red: 193, green: 154, blue: 118 },
            { red: 205, green: 190, blue: 169 },
            { red: 199, green: 111, blue: 69 },
            { red: 221, green: 223, blue: 228 },
            { red: 115, green: 92, blue: 87 },
            { red: 214, green: 165, blue: 137 },
            { red: 160, green: 124, blue: 33 },
            { red: 117, green: 91, blue: 86 },
            { red: 113, green: 104, blue: 107 },
            { red: 216, green: 153, blue: 99 },
            { red: 242, green: 226, blue: 64 }
  ];

  var SOUND_BASE_URL = "http://public.highfidelity.io/sounds/Animals/";

  for (var i = 0; i < howMany; i++) {
    var whichBird = Math.floor(Math.random() * sound_filenames.length);
    var position = {
        x: lowerCorner.x + (upperCorner.x - lowerCorner.x) / 2.0 + (Math.random() - 0.5) * (upperCorner.x - lowerCorner.x) * STARTING_FRACTION,
        y: lowerCorner.y + (upperCorner.y - lowerCorner.y) / 2.0 + (Math.random() - 0.5) * (upperCorner.y - lowerCorner.y) * STARTING_FRACTION,
        z: lowerCorner.z + (upperCorner.z - lowerCorner.x) / 2.0 + (Math.random() - 0.5) * (upperCorner.z - lowerCorner.z) * STARTING_FRACTION
    };

    birds.push({
        sound: SoundCache.getSound(SOUND_BASE_URL + sound_filenames[whichBird]),
        entityId: Entities.addEntity({
                    type: "Sphere",
                    position: position,
                    dimensions: { x: BIRD_SIZE, y: BIRD_SIZE, z: BIRD_SIZE },
                    gravity: {  x: 0, y: BIRD_GRAVITY, z: 0 },
                    velocity: { x: 0, y: -0.1, z: 0 },
                    damping: LINEAR_DAMPING,
                    dynamic: true,
                    lifetime: STARTING_LIFETIME,
                    color: colors[whichBird]
        }),
        audioId: false,
        isPlaying: false
      });
    }
    if (MAKE_FLOOR) {
        var FLOOR_THICKNESS = 0.05;
        floor = Entities.addEntity({ type: "Box", position: {   x: lowerCorner.x + (upperCorner.x - lowerCorner.x) / 2.0,
                                                                y: lowerCorner.y,
                                                                z: lowerCorner.z + (upperCorner.z - lowerCorner.z) / 2.0  },
                                                   dimensions: { x: (upperCorner.x - lowerCorner.x), y: FLOOR_THICKNESS, z: (upperCorner.z - lowerCorner.z)},
                                                   color: {red: 100, green: 100, blue: 100}
                                    });
    }
}
