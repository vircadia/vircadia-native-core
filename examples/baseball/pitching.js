Script.include("line.js");

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
        x: 0,
        y: 0.8,
        z: -18.3,
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
    lifetime: 20,
    //collisionSoundURL: PITCH_THUNK_SOUND_URL,
    gravity: {
        x: 0,
        y: 0,//-9.8,
        z: 0
    }
};
var BASEBALL_STATE = {
    PITCHING: 0,
    HIT: 1,
    STRIKE: 2,
    FOUL: 3
};


var pitchingMachineID = Entities.addEntity(PITCHING_MACHINE_PROPERTIES);

var pitchFromPosition = { x: 0, y: 1.0, z: 0 };
var pitchDirection = { x: 0, y: 0, z: 1 };

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

var injector = null;

var ACCELERATION_SPREAD = 10.15;

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
    trail = new InfiniteLine(position, { red: 255, green: 128, blue: 89 });
    trailInterval = Script.setInterval(function() {
        var properties = Entities.getEntityProperties(entityID, ['position']);
        if (Vec3.distance(properties.position, lastPosition)) {
            trail.enqueuePoint(properties.position);
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
        x: randomInt(-ACCELERATION_SPREAD, ACCELERATION_SPREAD),
        y: randomInt(-ACCELERATION_SPREAD, ACCELERATION_SPREAD),
        z: 0.0,
    };
    */

    // Create entity
    this.entityID = Entities.addEntity(properties);

    // Listen for collision for the lifetime of the entity
    Script.addEventHandler(this.entityID, "collisionWithEntity", function(entityA, entityB, collision) {
         self.collisionCallback(entityA, entityB, collision);
     });
    /*
    if (false && Math.random() < 0.5) {
        for (var i = 0; i < 50; i++) {
            Script.setTimeout(function() {
                Entities.editEntity(entityID, {
                    gravity: {
                        x: randomInt(-ACCELERATION_SPREAD, ACCELERATION_SPREAD),
                        y: randomInt(-ACCELERATION_SPREAD, ACCELERATION_SPREAD),
                        z: 0.0,
                    }
                })
            }, i * 100);
        }
    }
    */
}

Baseball.prototype = {
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
        print("Hit: " + name);
        if (name == "Bat") {
            if (this.state == BASEBALL_STATE.PITCHING) {
                print("HIT");

                // Update ball velocity
                Entities.editEntity(self.entityID, {
                    velocity: Vec3.multiply(2, myVelocity),
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
                var yaw = Math.atan2(myVelocity.x, myVelocity.z) * 180 / Math.PI;
                var foul = yaw > -135 && yaw < 135;
                if (foul) {
                    print("FOUL ", yaw)
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
            print("PARTICLES");
            entityCollisionWithGround(entityB, this.entityID, collision);
        } else if (name == "backstop") {
            if (this.state == BASEBALL_STATE.PITCHING) {
                this.state = BASEBALL_STATE.STRIKE;
                print("STRIKE");
                playRandomSound(AUDIO.strike, {
                    position: myPosition,
                    volume: 2.0
                });
            }
        }
    }
}

function pitchBall() {
    var machineProperties = Entities.getEntityProperties(pitchingMachineID, ["dimensions", "position", "rotation"]);
    var pitchFromPositionBase = machineProperties.position;
    var pitchFromOffset = vec3Mult(machineProperties.dimensions, PITCHING_MACHINE_OUTPUT_OFFSET_PCT);
    pitchFromOffset = Vec3.multiplyQbyV(machineProperties.rotation, pitchFromOffset);
    var pitchFromPosition = Vec3.sum(pitchFromPositionBase, pitchFromOffset);
    var pitchDirection = Quat.getFront(machineProperties.rotation);
    var ballScale = machineProperties.dimensions.x / PITCHING_MACHINE_PROPERTIES.dimensions.x;
    print("Creating baseball");

    var speed = randomFloat(BASEBALL_MIN_SPEED, BASEBALL_MAX_SPEED)
    var timeToPassPlate = (DISTANCE_FROM_PLATE + 1.0) / speed;

    var baseball = new Baseball(pitchFromPosition, Vec3.multiply(speed, pitchDirection), ballScale);

    if (!injector) {
        injector = Audio.playSound(pitchSound, {
            position: pitchFromPosition,
            volume: 1.0
        });
    } else {
        injector.restart();
    }
}

function entityCollisionWithGround(ground, entity, collision) {
    var ZERO_VEC = { x: 0, y: 0, z: 0 };
    var dVelocityMagnitude = Vec3.length(collision.velocityChange);
    var position = Entities.getEntityProperties(entity, "position").position;
    var particleRadius = 0.3;//map(dVelocityMagnitude, 0.05, 3, 0.5, 2);
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

Script.setInterval(pitchBall, PITCH_RATE);
