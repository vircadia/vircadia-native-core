var fishTank, bubbleSystem, secondBubbleSystem, innerContainer, bubbleInjector, lowerCorner, upperCorner, urchin, rocks;

var CLEANUP = true;
var TANK_DIMENSIONS = {
    x: 1.3393,
    y: 1.3515,
    z: 3.5914
};

var INNER_TANK_SCALE = 0.7;
var INNER_TANK_DIMENSIONS = Vec3.multiply(INNER_TANK_SCALE, TANK_DIMENSIONS);
INNER_TANK_DIMENSIONS.y = INNER_TANK_DIMENSIONS.y - 0.4;
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

var BUBBLE_SYSTEM_FORWARD_OFFSET = TANK_DIMENSIONS.x;
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

var URCHIN_FORWARD_OFFSET = -TANK_DIMENSIONS.x;
//depth of tank
var URCHIN_LATERAL_OFFSET = -0.15;
var URCHIN_VERTICAL_OFFSET = -0.37;

var URCHIN_MODEL_URL = 'http://hifi-content.s3.amazonaws.com/DomainContent/Home/fishTank/Urchin.fbx';

var URCHIN_DIMENSIONS = {
    x: 0.35,
    y: 0.35,
    z: 0.35
}

var ROCKS_FORWARD_OFFSET = (TANK_DIMENSIONS.x / 2) - 0.75;
//depth of tank
var ROCKS_LATERAL_OFFSET = 0.0;
var ROCKS_VERTICAL_OFFSET = (-TANK_DIMENSIONS.y / 2) + 0.25;

var ROCK_MODEL_URL = 'http://hifi-content.s3.amazonaws.com/DomainContent/Home/fishTank/Aquarium-Rocks-2.fbx';

var ROCK_DIMENSIONS = {
    x: 0.88,
    y: 0.33,
    z: 2.9
}

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
            "y": TANK_DIMENSIONS.y,
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

    var finalOffset = getOffsetFromTankCenter(BUBBLE_SYSTEM_VERTICAL_OFFSET, BUBBLE_SYSTEM_FORWARD_OFFSET, BUBBLE_SYSTEM_LATERAL_OFFSET);

    bubbleProperties.position = finalOffset;

    bubbleSystem = Entities.addEntity(bubbleProperties);
    createBubbleSound(finalOffset);
}

function createSecondBubbleSystem() {

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
            "x": -0.4,
            "y": TANK_DIMENSIONS.y,
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

    var finalOffset = getOffsetFromTankCenter(BUBBLE_SYSTEM_VERTICAL_OFFSET, BUBBLE_SYSTEM_FORWARD_OFFSET, BUBBLE_SYSTEM_LATERAL_OFFSET - 0.076);

    bubbleProperties.position = finalOffset;

    secondBubbleSystem = Entities.addEntity(bubbleProperties);

}



function getOffsetFromTankCenter(VERTICAL_OFFSET, FORWARD_OFFSET, LATERAL_OFFSET) {

    var tankProperties = Entities.getEntityProperties(fishTank);

    var upVector = Quat.getUp(tankProperties.rotation);
    var frontVector = Quat.getFront(tankProperties.rotation);
    var rightVector = Quat.getRight(tankProperties.rotation);

    var upOffset = Vec3.multiply(upVector, VERTICAL_OFFSET);
    var frontOffset = Vec3.multiply(frontVector, FORWARD_OFFSET);
    var rightOffset = Vec3.multiply(rightVector, LATERAL_OFFSET);

    var finalOffset = Vec3.sum(tankProperties.position, upOffset);
    finalOffset = Vec3.sum(finalOffset, frontOffset);
    finalOffset = Vec3.sum(finalOffset, rightOffset);
    return finalOffset
}

function createBubbleSound(position) {
    var audioProperties = {
        volume: 0.05,
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
        dynamic: false
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
        position: bounds.brn,
        visible: false
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
        position: bounds.tfl,
        visible: false
    }

    lowerCorner = Entities.addEntity(lowerProps);
    upperCorner = Entities.addEntity(upperProps);

}

function createRocks() {
    var finalPosition = getOffsetFromTankCenter(ROCKS_VERTICAL_OFFSET, ROCKS_FORWARD_OFFSET, ROCKS_LATERAL_OFFSET);

    var properties = {
        name: 'hifi-home-fishtank-rock',
        type: 'Model',
        parentID: fishTank,
        modelURL: ROCK_MODEL_URL,
        position: finalPosition,
        dimensions: ROCK_DIMENSIONS
    }

    rocks = Entities.addEntity(properties);
}

function createUrchin() {
    var finalPosition = getOffsetFromTankCenter(URCHIN_VERTICAL_OFFSET, URCHIN_FORWARD_OFFSET, URCHIN_LATERAL_OFFSET);

    var properties = {
        name: 'hifi-home-fishtank-urchin',
        type: 'Model',
        parentID: fishTank,
        modelURL: URCHIN_MODEL_URL,
        position: finalPosition,
        shapeType: 'Sphere',
        dimensions: URCHIN_DIMENSIONS
    }

    urchin = Entities.addEntity(properties);

}


createFishTank();

createInnerContainer();

createBubbleSystem();

createSecondBubbleSystem();

createEntitiesAtCorners();



createUrchin();

createRocks();

var customKey = 'hifi-home-fishtank';


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

//fisthank initialize has a different UUID than the model that i see
Script.setTimeout(function() {
    print('CREATE TIMEOUT!!!')
    print('TANK AT CREATE IS::: ' + fishTank)
    print('DATA AT CREATE IS:::' + JSON.stringify(data));

    setEntityCustomData(customKey, fishTank, data);
    // setEntityCustomData('grabbableKey', id, {
    //     grabbable: false
    // });
}, 2000)


function cleanup() {
    Entities.deleteEntity(fishTank);
    Entities.deleteEntity(bubbleSystem);
    Entities.deleteEntity(secondBubbleSystem);
    Entities.deleteEntity(innerContainer);
    Entities.deleteEntity(lowerCorner);
    Entities.deleteEntity(upperCorner);
    Entities.deleteEntity(urchin);
    Entities.deleteEntity(rocks);
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