var fishtank, bubbleSystem, bubbleSound, bubbleInjector;

var CLEANUP = true;
var TANK_DIMENSIONS = {
    x: 1.3393,
    y: 1.3515,
    z: 3.5914
};

var INNER_TANK_SCALE = 0.8;
var INNER_TANK_DIMENSIONS = Vec3.multiply(INNER_TANK_SCALE,TANK_DIMENSIONS);
var TANK_WIDTH = TANK_DIMENSIONS.z;
var TANK_HEIGHT = TANK_DIMENSIONS.y;

var DEBUG_COLOR = {
    red: 255,
    green: 0,
    blue: 255
}

var center = Vec3.sum(MyAvatar.position, Vec3.multiply(Quat.getFront(MyAvatar.orientation), 1 * TANK_WIDTH));

var TANK_POSITION = center;

var TANK_SCRIPT = Script.resolvePath('tank.js?' + Math.random())

var TANK_MODEL_URL = "http://hifi-content.s3.amazonaws.com/DomainContent/Home/fishTank/aquarium-6.fbx";

var BUBBLE_SYSTEM_FORWARD_OFFSET = 0.0;
var BUBBLE_SYSTEM_LATERAL_OFFSET = TANK_DIMENSIONS.x - 0.25;
var BUBBLE_SYSTEM_VERTICAL_OFFSET = -1;

var BUBBLE_SYSTEM_DIMENSIONS = {
    x: TANK_DIMENSIONS.x / 8,
    y: TANK_DIMENSIONS.y,
    z: TANK_DIMENSIONS.z / 8
}

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

    var tankProperties = Entities.getEntityProperties(fishtank);
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
    bubbleProperties.parentID = fishTank;
    bubbleProperties.dimensions = BUBBLE_SYSTEM_DIMENSIONS;

    var upVector = Quat.getRight(tankProperties.rotation);
    var frontVector = Quat.getRight(tankProperties.rotation);
    var rightVector = Quat.getRight(tankProperties.rotation);

    var upOffset = Vec3.multiply(upVector, BUBBLE_SYSTEM_VERTICAL_OFFSET);
    var frontOffset = Vec3.multiply(frontVector, BUBBLE_SYSTEM_FORWARD_OFFSET);
    var rightOffset = Vec3.multiply(rightVector, BUBBLE_SYSTEM_LATERAL_OFFSET);

    var finalOffset = Vec3.sum(center, upOffset);
    finalOffset = Vec3.sum(finalOffset, frontOffset);
    finalOffset = Vec3.sum(finalOffset, rightOffset);

    print('final bubble offset:: ' + JSON.stringify(finalOffset));
    bubbleProperties.position = finalOffset;

    bubbleSystem = Entities.addEntity(bubbleProperties);
   // createBubbleSound(finalOffset);
}

function createBubbleSound(position) {
    var audioProperties = {
        volume: 1,
        position: position,
        loop: true
    };

    bubbleInjector = Audio.playSound(bubbleSound, audioProperties);

}

function cleanup() {
    Entities.deleteEntity(fishTank);
    Entities.deleteEntity(bubbleSystem);
    bubbleInjector.stop();
    bubbleInjector = null;
}

function createInnerContainer(){
    var containerProps = {
        name:"hifi-home-fishtank-inner-container",
        type:'Box',
        color:{
            red:0,
            green:0,
            blue:255
        },
        dimensions:{

        }
    };
}

function createEntitiesAtCorners() {

    var bounds = Entities.getEntityProperties(fishTank, "boundingBox").boundingBox;

    var lowerProps = {
        name:'hifi-home-fishtank-lower-corner',
        type: "Box",
        dimensions: {
            x: 0.2,
            y: 0.2,
            z: 0.2
        },
        color: {
            red: 255,
            green: 0,
            blue: 0
        },
        collisionless: true,
        position: bounds.brn
    }

    var upperProps = {
        name:'hifi-home-fishtank-upper-corner',
        type: "Box",
        dimensions: {
            x: 0.2,
            y: 0.2,
            z: 0.2
        },
        color: {
            red: 0,
            green: 255,
            blue: 0
        },
        collisionless: true,
        position:bounds.tfl
    }

    var lowerCorner = Entities.addEntity(lowerProps);
    var upperCorner = Entities.addEntity(upperProps);
    print('CORNERS :::' + JSON.stringify(upperCorner) )
     print('CORNERS :::' + JSON.stringify(lowerCorner) )
}


createFishTank();

createBubbleSystem();

createEntitiesAtCorners();
//createBubbleSound();



if (CLEANUP === true) {
    Script.scriptEnding.connect(cleanup);
}