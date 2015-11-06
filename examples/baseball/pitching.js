print("Loading pitching");
//Script.include("../libraries/line.js");
Script.include("https://raw.githubusercontent.com/huffman/hifi/line-js/examples/libraries/line.js");
Script.include("http://rawgit.com/huffman/hifi/baseball/examples/baseball/firework.js");

function findEntity(properties, searchRadius) {
    var entities = findEntities(properties, searchRadius);
    return entities.length > 0 ? entities[0] : null;
}

// Return all entities with properties `properties` within radius `searchRadius`
function findEntities(properties, searchRadius) {
    var entities = Entities.findEntities(MyAvatar.position, searchRadius);
    var matchedEntities = [];
    var keys = Object.keys(properties);
    for (var i = 0; i < entities.length; ++i) {
        var match = true;
        var candidateProperties = Entities.getEntityProperties(entities[i], keys);
        for (var key in properties) {
            if (candidateProperties[key] != properties[key]) {
                // This isn't a match, move to next entity
                match = false;
                break;
            }
        }
        if (match) {
            matchedEntities.push(entities[i]);
        }
    }

    return matchedEntities;
}

var DISTANCE_BILLBOARD_NAME = "CurrentScore";
var HIGH_SCORE_BILLBOARD_NAME = "HighScore";
var DISTANCE_BILLBOARD_ENTITY_ID = findEntity({name: DISTANCE_BILLBOARD_NAME }, 1000);
var HIGH_SCORE_BILLBOARD_ENTITY_ID = findEntity({name: HIGH_SCORE_BILLBOARD_NAME }, 1000);

print("Distance: ", DISTANCE_BILLBOARD_ENTITY_ID)

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
        z: -19.8,
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
        z: 0.5,
    },
    rotation: Quat.fromPitchYawRollDegrees(0, 180, 0),
    modelURL: PITCHING_MACHINE_URL,
    dimensions: {
        x: 0.4,
        y: 0.61,
        z: 0.39
    },
    collisionsWillMove: false,
    shapeType: "Box",
};
PITCHING_MACHINE_PROPERTIES.dimensions = Vec3.multiply(2.5, PITCHING_MACHINE_PROPERTIES.dimensions);
var DISTANCE_FROM_PLATE = PITCHING_MACHINE_PROPERTIES.position.z;

var PITCH_RATE = 5000;

getPitchingMachine = function() {
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
        var pitchDirection = { x: 0, y: 0, z: 1 };
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
        print("Created baseball");
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
                Script.setTimeout(function() { self.pitchBall() }, 3000);
            }
        }
    }
};

var BASEBALL_MODEL_URL = "atp:7185099f1f650600ca187222573a88200aeb835454bd2f578f12c7fb4fd190fa.fbx";
var BASEBALL_MIN_SPEED = 2.7;
var BASEBALL_MAX_SPEED = 5.7;
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
    collisionsWillMove: true,
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


function shallowCopy(obj) {
    var copy = {}
    for (var key in obj) {
        copy[key] = obj[key];
    }
    return copy;
}

function randomInt(low, high) {
    return Math.floor(randomFloat(low, high));
}

function randomFloat(low, high) {
    if (high === undefined) {
        high = low;
        low = 0;
    }
    return low + Math.random() * (high - low);
}

function playRandomSound(sounds, options) {
    if (options === undefined) {
        options = {
            volume: 1.0,
            position: MyAvatar.position,
        }
    }
    Audio.playSound(sounds[randomInt(sounds.length)], options);
}

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

function ObjectTrail(entityID, startPosition) {
    this.entityID = entityID;
    this.line = null;
    var lineInterval = null;

    var lastPosition = startPosition;
    trail = new InfiniteLine(startPosition, trailColor, trailLifetime);
    trailInterval = Script.setInterval(function() {
        var properties = Entities.getEntityProperties(entityID, ['position']);
        if (Vec3.distance(properties.position, lastPosition)) {
            var strokeWidth = Math.log(1 + trail.size) * 0.05;
            trail.enqueuePoint(properties.position, strokeWidth);
            lastPosition = properties.position;
        }
    }, 50);
}

ObjectTrail.prototype = {
    destroy: function() {
        if (this.line) {
            Script.clearInterval(this.lineInterval);
            this.lineInterval = null;

            this.line.destroy();
            this.line = null;
        }
    }
};

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

// Update the stadium billboard with the current distance and update the high score
// if it has been beaten.
function updateBillboard(distance) {
    Entities.editEntity(DISTANCE_BILLBOARD_ENTITY_ID, {
        text: distance,
    });

    // If a number was passed in, let's see if it is larger than the current high score
    // and update it if so.
    if (!isNaN(distance)) {
        var properties = Entities.getEntityProperties(HIGH_SCORE_BILLBOARD_ENTITY_ID, ["text"]);
        var bestDistance = parseInt(properties.text);
        if (distance >= bestDistance) {
            Entities.editEntity(HIGH_SCORE_BILLBOARD_ENTITY_ID, {
                text: distance,
            });
            return true;
        }
    }
    return false;
}

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
            playFireworkShow(numberOfFireworks, 2000);
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
                var yaw = Math.atan2(myVelocity.x, myVelocity.z) * 180 / Math.PI;
                var foul = yaw > -135 && yaw < 135;

                var speedMultiplier = 2;

                if (foul && myVelocity.z > 0) {
                    var xzDist = Math.sqrt(myVelocity.x * myVelocity.x + myVelocity.z * myVelocity.z);
                    var pitch = Math.atan2(myVelocity.y, xzDist) * 180 / Math.PI;
                    print("Pitch: ", pitch);
                    if (Math.abs(pitch) < 15) {
                        print("Reversing hit");
                        myVelocity.z *= -1;
                        myVelocity.y *= -1;
                        Vec3.length(myVelocity);
                        foul = false;
                        speedMultiplier = 10;
                    }
                }

                // Update ball velocity
                Entities.editEntity(self.entityID, {
                    velocity: Vec3.multiply(speedMultiplier, myVelocity),
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

var baseball = null;

function update(dt) {
    if (baseball) {
        baseball.update(dt);
        if (baseball.finished()) {
            baseball = null;
            Script.setTimeout(pitchBall, 3000);
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

//Script.update.connect(update);
//var pitchingMachine = getPitchingMachine();
//pitchingMachine.pitchBall();
//Script.update.connect(function(dt) { pitchingMachine.update(dt); });

print("Done loading pitching.js");
