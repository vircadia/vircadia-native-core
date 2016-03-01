var fishtank, bubbleSystem, bubbleSound;

var TANK_DIMENSIONS = {
    x: 1.3393,
    y: 1.3515,
    z: 3.5914
};

var TANK_WIDTH = TANK_DIMENSIONS.z;
var TANK_HEIGHT = TANK_DIMENSIONS.y;

var DEBUG_COLOR = {
    red: 255,
    green: 0,
    blue: 255
}

var center = Vec3.sum(MyAvatar.position, Vec3.multiply(Quat.getFront(MyAvatar.orientation), 2 * TANK_WIDTH));

var TANK_POSITION = center;

var TANK_SCRIPT = Script.resolvePath('tank.js?' + Math.random())

var TANK_MODEL_URL = "http://hifi-content.s3.amazonaws.com/DomainContent/Home/fishTank/aquarium-6.fbx";

var BUBBLE_SYSTEM_FORWARD_OFFSET = 0.2;
var BUBBLE_SYSTEM_LATERAL_OFFSET = 0.2;
var BUBBLE_SYSTEM_VERTICAL_OFFSET = -0.2;

var BUBBLE_SOUND_URL = "http://hifi-content.s3.amazonaws.com/DomainContent/Home/Sounds/aquarium_small.L.wav";
var bubbleSound = SoundCache.getSound(BUBBLE_SOUND_URL);

function createFishTank() {
    var tankProperties = {
        name: 'hifi-home-fishtank',
        type: 'Model',
        modelURL: TANK_MODEL_URL,
        dimensions: TANK_DIMENSIONS,
        position: TANK_POSITION,
        color: DEBUG_COLOR,
        collisionless: true,
        script: TANK_SCRIPT,
        userData: JSON.stringify({
            'hifi-home-fishtank': {
                fishLoaded: false,
                bubbleSystem: null,
                bubbleSound: null,
                attractors: null,
            },
            grabbableKey: {
                grabbable: false
            }
        }),
        visible: true
    }

    fishTank = Entities.addEntity(tankProperties);
}

function createBubbleSystem() {

    var tankProperties = Entities.getEntityProperties(fishTank);
    var bubbleProperties = {
        "color": {},
        "isEmitting": 1,
        "maxParticles": 1880,
        "lifespan": 1.6,
        "emitRate": 10,
        "emitSpeed": 0.025,
        "speedSpread": 0.025,
        "emitOrientation": {
            "x": 0,
            "y": 0.5,
            "z": 0.5,
            "w": 0
        },
        "emitDimensions": {
            "x": -0.2,
            "y": 1.2000000000000002,
            "z": 0
        },
        "polarStart": 0,
        "polarFinish": 0,
        "azimuthStart": 0.2,
        "azimuthFinish": 0.1,
        "emitAcceleration": {
            "x": 0,
            "y": 0.4,
            "z": 0
        },
        "accelerationSpread": {
            "x": 0.1,
            "y": 0.1,
            "z": 0.1
        },
        "particleRadius": 0.02,
        "radiusSpread": 0,
        "radiusStart": 0.043,
        "radiusFinish": 0.02,
        "colorSpread": {},
        "colorStart": {},
        "colorFinish": {},
        "alpha": 0.2,
        "alphaSpread": 0,
        "alphaStart": 0.3,
        "alphaFinish": 0,
        "emitterShouldTrail": 0,
        "textures": "http://hifi-content.s3.amazonaws.com/DomainContent/Home/fishTank/bubble-white.png"
    };

    bubbleProperties.type = "ParticleEffect";
    bubbleProperties.collisionless = true;

    var upVector = Quat.getRight(tankProperties.rotation);
    var frontVector = Quat.getRight(tankProperties.rotation);
    var rightVector = Quat.getRight(tankProperties.rotation);

    var upOffset = Vec3.multiply(upVector, BUBBLE_SYSTEM_VERTICAL_OFFSET);
    var frontOffset = Vec3.multiply(frontVector, BUBBLE_SYSTEM_FORWARD_OFFSET);
    var rightOffset = Vec3.multiply(rightVector, BUBBLE_SYSTEM_LATERAL_OFFSET);

    var finalOffset = Vec3.sum(center, upOffset);
    finalOffset = Vec3.sum(finalOffset, frontOffset);
    finalOffset = Vec3.sum(finalOffset, rightOffset);

    bubbleProperties.position = finalOffset;

    bubbleSystem = Entities.addEntity(bubbleProperties);
}

function createBubbleSound() {
    var bubbleSystemProperties = Entities.getEntityProperties(bubbleSystem);
    var audioProperties = {
        volume: 0.2,
        position: position
    };

    Audio.playSound(bubbleSound, audioProperties);

}

function cleanup() {
    Entities.deleteEntity(fishTank);
}

createFishTank();

// createBubbleSystem();

// createBubbleSound();

// createAttractors();

var attractors = []
    //@position,radius,strength



function createAttractor(position, radius, strength) {
    return {
        position: position,
        radius: radius,
        strength: strength
    };
}


Script.scriptEnding.connect(cleanup);