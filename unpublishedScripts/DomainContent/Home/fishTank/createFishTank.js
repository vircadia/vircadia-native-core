var fishTank, bubbleSystem, innerContainer, bubbleInjector, lowerCorner, upperCorner;

var CLEANUP = true;
var TANK_DIMENSIONS = {
    x: 1.3393,
    y: 1.3515,
    z: 3.5914
};

var INNER_TANK_SCALE = 0.7;
var INNER_TANK_DIMENSIONS = Vec3.multiply(INNER_TANK_SCALE, TANK_DIMENSIONS);
INNER_TANK_DIMENSIONS.y = INNER_TANK_DIMENSIONS.y -0.25;
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

var BUBBLE_SYSTEM_FORWARD_OFFSET = TANK_DIMENSIONS.x - 0.05;
//depth of tank
var BUBBLE_SYSTEM_LATERAL_OFFSET = 0.15;
var BUBBLE_SYSTEM_VERTICAL_OFFSET = -0.5;

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
        visible: true
    }

    fishTank = Entities.addEntity(tankProperties);
}

function createBubbleSystem() {

    var tankProperties = Entities.getEntityProperties(fishTank);
    var bubbleProperties = {
        "name": 'hifi-home-fishtank-bubbles',
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

    var upVector = Quat.getUp(tankProperties.rotation);
    var frontVector = Quat.getFront(tankProperties.rotation);
    var rightVector = Quat.getRight(tankProperties.rotation);

    var upOffset = Vec3.multiply(upVector, BUBBLE_SYSTEM_VERTICAL_OFFSET);
    var frontOffset = Vec3.multiply(frontVector, BUBBLE_SYSTEM_FORWARD_OFFSET);
    var rightOffset = Vec3.multiply(rightVector, BUBBLE_SYSTEM_LATERAL_OFFSET);

    var finalOffset = Vec3.sum(tankProperties.position, upOffset);
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

function createInnerContainer(position) {

    var tankProperties = Entities.getEntityProperties(fishTank);

    var containerProps = {
        name: "hifi-home-fishtank-inner-container",
        type: 'Box',
        color: {
            red: 0,
            green: 0,
            blue: 255
        },
        parentID: fishTank,
        dimensions: INNER_TANK_DIMENSIONS,
        position: tankProperties.position,
        visible: false,
        collisionless: true,
        dynamic:false
    };

    innerContainer = Entities.addEntity(containerProps);
}

function createEntitiesAtCorners() {

    var bounds = Entities.getEntityProperties(innerContainer, "boundingBox").boundingBox;

    var lowerProps = {
        name: 'hifi-home-fishtank-lower-corner',
        type: "Box",
        parentID: fishTank,
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
        name: 'hifi-home-fishtank-upper-corner',
        type: "Box",
        parentID: fishTank,
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
        position: bounds.tfl
    }

    lowerCorner = Entities.addEntity(lowerProps);
    upperCorner = Entities.addEntity(upperProps);
    print('CORNERS :::' + JSON.stringify(upperCorner))
    print('CORNERS :::' + JSON.stringify(lowerCorner))
}


createFishTank();

createInnerContainer();

createBubbleSystem();

createEntitiesAtCorners();

createBubbleSound();

var customKey = 'hifi-home-fishtank';
var id = fishTank;
print('FISH TANK ID AT START:: '+id)
var data = {
        fishLoaded: false,
        bubbleSystem: bubbleSystem,
        bubbleSound: bubbleSound,
        corners: {
            brn: lowerCorner,
            tfl: upperCorner
        },
        innerContainer: innerContainer,

    }
    // print('DATA AT CREATE IS:::' + JSON.stringify(data));
setEntityCustomData(customKey, id, data);
setEntityCustomData('grabbableKey', id, {
    grabbable: false
});

function cleanup() {
    Entities.deleteEntity(fishTank);
    Entities.deleteEntity(bubbleSystem);
    Entities.deleteEntity(innerContainer);
    bubbleInjector.stop();
    bubbleInjector = null;
}


if (CLEANUP === true) {
    Script.scriptEnding.connect(cleanup);
}

//  Copyright 2016 High Fidelity, Inc.
//
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

function setEntityUserData(id, data) {
    var json = JSON.stringify(data)
    Entities.editEntity(id, {
        userData: json
    });
}

// FIXME do non-destructive modification of the existing user data
function getEntityUserData(id) {
    var results = null;
    var properties = Entities.getEntityProperties(id, "userData");
    if (properties.userData) {
        try {
            results = JSON.parse(properties.userData);
        } catch (err) {
            //   print('error parsing json');
            //   print('properties are:'+ properties.userData);
        }
    }
    return results ? results : {};
}


// Non-destructively modify the user data of an entity.
function setEntityCustomData(customKey, id, data) {
    var userData = getEntityUserData(id);
    if (data == null) {
        delete userData[customKey];
    } else {
        userData[customKey] = data;
    }
    setEntityUserData(id, userData);
}

function getEntityCustomData(customKey, id, defaultValue) {
    var userData = getEntityUserData(id);
    if (undefined != userData[customKey]) {
        return userData[customKey];
    } else {
        return defaultValue;
    }
}