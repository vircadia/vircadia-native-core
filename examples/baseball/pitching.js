//
//  pitching.js
//  examples/baseball/
//
//  Created by Ryan Huffman on Nov 9, 2015
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

print("Loading pitching");

Script.include("../libraries/line.js");
Script.include("firework.js");
Script.include("utils.js");

var DISTANCE_BILLBOARD_NAME = "CurrentScore";
var HIGH_SCORE_BILLBOARD_NAME = "HighScore";

var distanceBillboardEntityID = null;
var highScoreBillboardEntityID = null;

function getDistanceBillboardEntityID() {
    if (distanceBillboardEntityID === null) {
        distanceBillboardEntityID = findEntity({name: DISTANCE_BILLBOARD_NAME }, 1000);
    }
    return distanceBillboardEntityID;
}
function getHighScoreBillboardEntityID() {
    if (highScoreBillboardEntityID === null) {
        highScoreBillboardEntityID = findEntity({name: HIGH_SCORE_BILLBOARD_NAME }, 1000);
    }
    return highScoreBillboardEntityID;
}

var METERS_TO_FEET = 3.28084;

var AUDIO = {
    crowdBoos: [
        SoundCache.getSound("atp:c632c92b166ade60aa16b23ff1dfdf712856caeb83bd9311980b2d5edac821af.wav", false)
    ],
    crowdCheers: [
        SoundCache.getSound("atp:0821bf2ac60dd2f356dfdd948e8bb89c23984dc3584612f6c815765154f02cae.wav", false),
        SoundCache.getSound("atp:b8044401a846ed29f881a0b9b80cf1ba41f26327180c28fc9c70d144f9b70045.wav", false),
    ],
    batHit: [
        SoundCache.getSound("atp:6f0b691a0c9c9ece6557d97fe242b1faec4020fe26efc9c17327993b513c5fe5.wav", false),
        SoundCache.getSound("atp:5be5806205158ebdc5c3623ceb7ae73315028b51ffeae24292aff7042e3fa6a9.wav", false),
        SoundCache.getSound("atp:e68661374e2145c480809c26134782aad11e0de456c7802170c7abccc4028873.wav", false),
        SoundCache.getSound("atp:787e3c9af17dd3929527787176ede83d6806260e63ddd5a4cef48cd22e32c6f7.wav", false),
        SoundCache.getSound("atp:fc65383431a6238c7a4749f0f6f061f75a604ed5e17d775ab1b2955609e67ebb.wav", false),
    ],
    strike: [
        SoundCache.getSound("atp:2a258076a85fffde4ba04b5ddc1de9034c7ae7d2af8c5d93d4fed0bcdef3472a.wav", false),
        SoundCache.getSound("atp:518363524af3ed9b9ae4ca2ceee61f01aecd37e266a51c5a5f5487efe2520fd5.wav", false),
        SoundCache.getSound("atp:d51d38b089574acbdfdf53ef733bfb3ab41d848fb8c0b6659c7790a785240009.wav", false),
    ],
    foul: [
        SoundCache.getSound("atp:316fa18ff9eef457f670452b449a8dc5a41ccabd4e948781c50aaafaae63b0ab.wav", false),
        SoundCache.getSound("atp:c84d88352d38437edd7414b26dc74e567618712caeb59fec70822398b0c5a279.wav", false),
    ]
}

var PITCH_THUNK_SOUND_URL = "http://hifi-public.s3.amazonaws.com/sounds/ping_pong_gun/pong_sound.wav";
var pitchSound = SoundCache.getSound(PITCH_THUNK_SOUND_URL, false);
updateBillboard("");

var PITCHING_MACHINE_URL = "atp:87d4879530b698741ecc45f6f31789aac11f7865a2c3bec5fe9b061a182c80d4.fbx";
// This defines an offset to pitch a ball from with respect to the machine's position. The offset is a
// percentage of the machine's dimensions. So, { x: 0.5, y: -1.0, z: 0.0 } would offset on 50% on the
// machine's x axis, -100% on the y axis, and 0% on the z-axis. For the dimensions { x: 100, y: 100, z: 100 },
// that would result in an offset of { x: 50, y: -100, z: 0 }. This makes it easy to calculate an offset if
// the machine's dimensions change.
var PITCHING_MACHINE_OUTPUT_OFFSET_PCT = {
    x: 0.0,
    y: 0.25,
    z: -1.05,
};
var PITCHING_MACHINE_PROPERTIES = {
    name: "Pitching Machine",
    type: "Model",
    position: {
        x: -0.93,
        y: 0.8,
        z: -19.8
    },
    velocity: {
        x: 0,
        y: -0.01,
        z: 0
    },
    gravity: {
        x: 0.0,
        y: -9.8,
        z: 0.0
    },
    registrationPoint: {
        x: 0.5,
        y: 0.5,
        z: 0.5
    },
    rotation: Quat.fromPitchYawRollDegrees(0, 180, 0),
    modelURL: PITCHING_MACHINE_URL,
    dimensions: {
        x: 0.4,
        y: 0.61,
        z: 0.39
    },
    dynamic: false,
    shapeType: "Box"
};
PITCHING_MACHINE_PROPERTIES.dimensions = Vec3.multiply(2.5, PITCHING_MACHINE_PROPERTIES.dimensions);
var DISTANCE_FROM_PLATE = PITCHING_MACHINE_PROPERTIES.position.z;

var PITCH_RATE = 5000;

getOrCreatePitchingMachine = function() {
    // Search for pitching machine
    var entities = findEntities({ name: PITCHING_MACHINE_PROPERTIES.name }, 1000);
    var pitchingMachineID = null;

    // Create if it doesn't exist
    if (entities.length == 0) {
        pitchingMachineID = Entities.addEntity(PITCHING_MACHINE_PROPERTIES);
    } else {
        pitchingMachineID = entities[0];
    }

    // Wrap with PitchingMachine object and return
    return new PitchingMachine(pitchingMachineID);
}

// The pitching machine wraps an entity ID and uses it's position & rotation to determin where to
// pitch the ball from and in which direction, and uses the dimensions to determine the scale of them ball.
function PitchingMachine(pitchingMachineID) {
    this.pitchingMachineID = pitchingMachineID;
    this.enabled = false;
    this.baseball = null;
    this.injector = null;
}

PitchingMachine.prototype = {
    pitchBall: function() {
        cleanupTrail();

        if (!this.enabled) {
            return;
        }

        print("Pitching ball");
        var machineProperties = Entities.getEntityProperties(this.pitchingMachineID, ["dimensions", "position", "rotation"]);
        var pitchFromPositionBase = machineProperties.position;
        var pitchFromOffset = vec3Mult(machineProperties.dimensions, PITCHING_MACHINE_OUTPUT_OFFSET_PCT);
        pitchFromOffset = Vec3.multiplyQbyV(machineProperties.rotation, pitchFromOffset);
        var pitchFromPosition = Vec3.sum(pitchFromPositionBase, pitchFromOffset);
        var pitchDirection = Quat.getFront(machineProperties.rotation);
        var ballScale = machineProperties.dimensions.x / PITCHING_MACHINE_PROPERTIES.dimensions.x;

        var speed = randomFloat(BASEBALL_MIN_SPEED, BASEBALL_MAX_SPEED);
        var velocity = Vec3.multiply(speed, pitchDirection);

        this.baseball = new Baseball(pitchFromPosition, velocity, ballScale);

        if (!this.injector) {
            this.injector = Audio.playSound(pitchSound, {
                position: pitchFromPosition,
                volume: 1.0
            });
        } else {
            this.injector.restart();
        }
    },
    start: function() {
        if (this.enabled) {
            return;
        }
        print("Starting Pitching Machine");
        this.enabled = true;
        this.pitchBall();
    },
    stop: function() {
        if (!this.enabled) {
            return;
        }
        print("Stopping Pitching Machine");
        this.enabled = false;
    },
    update: function(dt) {
        if (this.baseball) {
            this.baseball.update(dt);
            if (this.baseball.finished()) {
                this.baseball = null;
                var self = this;
                Script.setTimeout(function() { self.pitchBall(); }, 3000);
            }
        }
    }
};

var BASEBALL_MODEL_URL = "atp:7185099f1f650600ca187222573a88200aeb835454bd2f578f12c7fb4fd190fa.fbx";
var BASEBALL_MIN_SPEED = 7.0;
var BASEBALL_MAX_SPEED = 15.0;
var BASEBALL_RADIUS = 0.07468;
var BASEBALL_PROPERTIES = {
    name: "Baseball",
    type: "Model",
    modelURL: BASEBALL_MODEL_URL,
    shapeType: "Sphere",
    position: {
        x: 0,
        y: 0,
        z: 0
    },
    dimensions: {
        x: BASEBALL_RADIUS,
        y: BASEBALL_RADIUS,
        z: BASEBALL_RADIUS
    },
    dynamic: true,
    angularVelocity: {
        x: 17.0,
        y: 0,
        z: -8.0,

        x: 0.0,
        y: 0,
        z: 0.0,
    },
    angularDamping: 0.0,
    damping: 0.0,
    restitution: 0.5,
    friction: 0.0,
    friction: 0.5,
    lifetime: 20,
    gravity: {
        x: 0,
        y: 0,
        z: 0
    }
};
var BASEBALL_STATE = {
    PITCHING: 0,
    HIT: 1,
    HIT_LANDED: 2,
    STRIKE: 3,
    FOUL: 4
};



function vec3Mult(a, b) {
    return {
        x: a.x * b.x,
        y: a.y * b.y,
        z: a.z * b.z,
    };
}

function map(value, min1, max1, min2, max2) {
    return min2 + (max2 - min2) * ((value - min1) / (max1 - min1));
}

function orientationOf(vector) {
    var RAD_TO_DEG = 180.0 / Math.PI;
    var Y_AXIS = { x: 0, y: 1, z: 0 };
    var X_AXIS = { x: 1, y: 0, z: 0 };
    var direction = Vec3.normalize(vector);

    var yaw = Quat.angleAxis(Math.atan2(direction.x, direction.z) * RAD_TO_DEG, Y_AXIS);
    var pitch = Quat.angleAxis(Math.asin(-direction.y) * RAD_TO_DEG, X_AXIS);

    return Quat.multiply(yaw, pitch);
}

var ACCELERATION_SPREAD = 0.35;

var TRAIL_COLOR = { red: 128, green: 255, blue: 89 };
var TRAIL_LIFETIME = 20;

var trail = null;
var trailInterval = null;
function cleanupTrail() {
    if (trail) {
        Script.clearInterval(this.trailInterval);
        trailInterval = null;

        trail.destroy();
        trail = null;
    }
}

function setupTrail(entityID, position) {
    cleanupTrail();

    var lastPosition = position;
    trail = new InfiniteLine(position, { red: 128, green: 255, blue: 89 }, 20);
    trailInterval = Script.setInterval(function() {
        var properties = Entities.getEntityProperties(entityID, ['position']);
        if (Vec3.distance(properties.position, lastPosition)) {
            var strokeWidth = Math.log(1 + trail.size) * 0.05;
            trail.enqueuePoint(properties.position, strokeWidth);
            lastPosition = properties.position;
        }
    }, 50);
}

function Baseball(position, velocity, ballScale) {
    var self = this;

    this.state = BASEBALL_STATE.PITCHING;

    // Setup entity properties
    var properties = shallowCopy(BASEBALL_PROPERTIES);
    properties.position = position;
    properties.velocity = velocity;
    properties.dimensions = Vec3.multiply(ballScale, properties.dimensions);
    /*
    properties.gravity = {
        x: randomFloat(-ACCELERATION_SPREAD, ACCELERATION_SPREAD),
        y: randomFloat(-ACCELERATION_SPREAD, ACCELERATION_SPREAD),
        z: 0.0,
    };
    */

    // Create entity
    this.entityID = Entities.addEntity(properties);

    this.timeSincePitched = 0;
    this.timeSinceHit = 0;
    this.hitBallAtPosition = null;
    this.distanceTravelled = 0;
    this.wasHighScore = false;
    this.landed = false;

    // Listen for collision for the lifetime of the entity
    Script.addEventHandler(this.entityID, "collisionWithEntity", function(entityA, entityB, collision) {
         self.collisionCallback(entityA, entityB, collision);
     });
    if (Math.random() < 0.5) {
        for (var i = 0; i < 50; i++) {
            Script.setTimeout(function() {
                Entities.editEntity(entityID, {
                    gravity: {
                        x: randomFloat(-ACCELERATION_SPREAD, ACCELERATION_SPREAD),
                        y: randomFloat(-ACCELERATION_SPREAD, ACCELERATION_SPREAD),
                        z: 0.0,
                    }
                })
            }, i * 100);
        }
    }
}

// Update the stadium billboard with the current distance or a text message and update the high score
// if it has been beaten.
function updateBillboard(distanceOrMessage) {
    var distanceBillboardEntityID = getDistanceBillboardEntityID();
    if (distanceBillboardEntityID) {
        Entities.editEntity(distanceBillboardEntityID, {
            text: distanceOrMessage
        });
    }

    var highScoreBillboardEntityID = getHighScoreBillboardEntityID();
    // If a number was passed in, let's see if it is larger than the current high score
    // and update it if so.
    if (!isNaN(distanceOrMessage) && highScoreBillboardEntityID) {
        var properties = Entities.getEntityProperties(highScoreBillboardEntityID, ["text"]);
        var bestDistance = parseInt(properties.text);
        if (distanceOrMessage >= bestDistance) {
            Entities.editEntity(highScoreBillboardEntityID, {
                text: distanceOrMessage,
            });
            return true;
        }
    }
    return false;
}

var FIREWORKS_SHOW_POSITION = { x: 0, y: 0, z: -78.0 };
var FIREWORK_PER_X_FEET = 100;
var MAX_FIREWORKS = 10;

Baseball.prototype = {
    finished: function() {
        return this.state == BASEBALL_STATE.FOUL
            || this.state == BASEBALL_STATE.STRIKE
            || this.state == BASEBALL_STATE.HIT_LANDED;
    },
    update: function(dt) {
        this.timeSincePitched += dt;
        if (this.state == BASEBALL_STATE.HIT) {
            this.timeSinceHit += dt;
            var myProperties = Entities.getEntityProperties(this.entityID, ['position', 'velocity']);
            var speed = Vec3.length(myProperties.velocity);
            this.distanceTravelled = Vec3.distance(this.hitBallAtPosition, myProperties.position) * METERS_TO_FEET;
            var wasHighScore = updateBillboard(Math.ceil(this.distanceTravelled));
            if (this.landed || this.timeSinceHit > 10 || speed < 1) {
                this.wasHighScore = wasHighScore;
                this.ballLanded();
            }
        } else if (this.state == BASEBALL_STATE.PITCHING) {
            if (this.timeSincePitched > 10) {
                print("TIMED OUT WHILE PITCHING");
                this.state = BASEBALL_STATE.STRIKE;
            }
        }
    },
    ballLanded: function() {
        this.state = BASEBALL_STATE.HIT_LANDED;
        var numberOfFireworks = Math.floor(this.distanceTravelled / FIREWORK_PER_X_FEET);
        if (numberOfFireworks > 0) {
            numberOfFireworks = Math.min(MAX_FIREWORKS, numberOfFireworks);
            playFireworkShow(FIREWORKS_SHOW_POSITION, numberOfFireworks, 2000);
        }
        print("Ball took " + this.timeSinceHit.toFixed(3) + " seconds to land");
        print("Ball travelled " + this.distanceTravelled + " feet")
    },
    collisionCallback: function(entityA, entityB, collision) {
        var self = this;
        var myProperties = Entities.getEntityProperties(this.entityID, ['position', 'velocity']);
        var myPosition = myProperties.position;
        var myVelocity = myProperties.velocity;

        // Activate gravity
        Entities.editEntity(self.entityID, {
            gravity: { x: 0, y: -9.8, z: 0 }
        });

        var name = Entities.getEntityProperties(entityB, ["name"]).name;
        if (name == "Bat") {
            if (this.state == BASEBALL_STATE.PITCHING) {
                print("HIT");

                var FOUL_MIN_YAW = -135.0;
                var FOUL_MAX_YAW = 135.0;

                var yaw = Math.atan2(myVelocity.x, myVelocity.z) * 180 / Math.PI;
                var foul = yaw > FOUL_MIN_YAW && yaw < FOUL_MAX_YAW;

                var speedMultiplier = 2;

                if (foul && myVelocity.z > 0) {
                    var TUNNELED_PITCH_RANGE = 15.0;
                    var xzDist = Math.sqrt(myVelocity.x * myVelocity.x + myVelocity.z * myVelocity.z);
                    var pitch = Math.atan2(myVelocity.y, xzDist) * 180 / Math.PI;
                    print("Pitch: ", pitch);
                    // If the pitch is going straight out the back and has a pitch in the range TUNNELED_PITCH_RANGE,
                    // let's assume the ball tunneled through the bat and reverse its direction.
                    if (Math.abs(pitch) < TUNNELED_PITCH_RANGE) {
                        print("Reversing hit");
                        myVelocity.x *= -1;
                        myVelocity.y *= -1;
                        myVelocity.z *= -1;

                        yaw = Math.atan2(myVelocity.x, myVelocity.z) * 180 / Math.PI;
                        foul = yaw > FOUL_MIN_YAW && yaw < FOUL_MAX_YAW;

                        speedMultiplier = 3;
                    }
                }

                // Update ball velocity
                Entities.editEntity(self.entityID, {
                    velocity: Vec3.multiply(speedMultiplier, myVelocity)
                });

                // Setup line update interval
                setupTrail(self.entityID, myPosition);

                // Setup bat hit sound
                playRandomSound(AUDIO.batHit, {
                    position: myPosition,
                    volume: 2.0
                });

                // Setup crowd reaction sound
                var speed = Vec3.length(myVelocity);
                Script.setTimeout(function() {
                    playRandomSound((speed < 5.0) ? AUDIO.crowdBoos : AUDIO.crowdCheers, {
                        position: { x: 0 ,y: 0, z: 0 },
                        volume: 1.0
                    });
                }, 500);

                if (foul) {
                    print("FOUL, yaw: ", yaw);
                    updateBillboard("FOUL");
                    this.state = BASEBALL_STATE.FOUL;
                    playRandomSound(AUDIO.foul, {
                        position: myPosition,
                        volume: 2.0
                    });
                } else {
                    print("HIT ", yaw);
                    this.state = BASEBALL_STATE.HIT;
                }
            }
        } else if (name == "stadium") {
            //entityCollisionWithGround(entityB, this.entityID, collision);
            this.landed = true;
        } else if (name == "backstop") {
            if (this.state == BASEBALL_STATE.PITCHING) {
                print("STRIKE");
                this.state = BASEBALL_STATE.STRIKE;
                updateBillboard("STRIKE");
                playRandomSound(AUDIO.strike, {
                    position: myPosition,
                    volume: 2.0
                });
            }
        }
    }
}

function entityCollisionWithGround(ground, entity, collision) {
    var ZERO_VEC = { x: 0, y: 0, z: 0 };
    var dVelocityMagnitude = Vec3.length(collision.velocityChange);
    var position = Entities.getEntityProperties(entity, "position").position;
    var particleRadius = 0.3;
    var speed = map(dVelocityMagnitude, 0.05, 3, 0.02, 0.09);
    var displayTime = 400;
    var orientationChange = orientationOf(collision.velocityChange);

    var dustEffect = Entities.addEntity({
        type: "ParticleEffect",
        name: "Dust-Puff",
        position: position,
        color: {red: 195, green: 170, blue: 185},
        lifespan: 3,
        lifetime: 2,//displayTime/1000 * 2, //So we can fade particle system out gracefully
        emitRate: 5,
        emitSpeed: speed,
        emitAcceleration: ZERO_VEC,
        accelerationSpread: ZERO_VEC,
        isEmitting: true,
        polarStart: Math.PI/2,
        polarFinish: Math.PI/2,
        emitOrientation: orientationChange,
        radiusSpread: 0.1,
        radiusStart: particleRadius,
        radiusFinish: particleRadius + particleRadius / 2,
        particleRadius: particleRadius,
        alpha: 0.45,
        alphaFinish: 0.001,
        textures: "https://hifi-public.s3.amazonaws.com/alan/Playa/Particles/Particle-Sprite-Gen.png"
    });
}

Script.scriptEnding.connect(function() {
    cleanupTrail();
    Entities.deleteEntity(pitchingMachineID);
});
