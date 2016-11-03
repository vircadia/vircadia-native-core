//
//  createTank.js
//
//
//  created by James b. Pollack @imgntn on 3/9/2016
//  Copyright 2016 High Fidelity, Inc.
//
//  Adds a fish tank and base, decorations, particle bubble systems, and a bubble sound.  Attaches a script that does fish swimming.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//
var TANK_SCRIPT = Script.resolvePath('entityLocalFish.js');

FishTank = function(spawnPosition, spawnRotation) {
    var fishTank, innerTank, tankBase, bubbleSystem, secondBubbleSystem, thirdBubbleSystem, anemone, treasure, rocks;
    var CLEANUP = true;

    var TANK_DIMENSIONS = {
        x: 0.8212,
        y: 0.8116,
        z: 2.1404
    };

    var INNER_TANK_SCALE = 0.85;
    var INNER_TANK_DIMENSIONS = Vec3.multiply(INNER_TANK_SCALE, TANK_DIMENSIONS);
    INNER_TANK_DIMENSIONS.y = INNER_TANK_DIMENSIONS.y - 0.2;
    var TANK_WIDTH = TANK_DIMENSIONS.z;
    var TANK_HEIGHT = TANK_DIMENSIONS.y;

    var DEBUG_COLOR = {
        red: 255,
        green: 0,
        blue: 255
    }

    var TANK_POSITION = spawnPosition;

    var TANK_MODEL_URL = "atp:/fishTank/aquariumTank.fbx";

    var TANK_BASE_MODEL_URL = 'atp:/fishTank/aquariumBase.fbx';

    var TANK_BASE_COLLISION_HULL = 'atp:/fishTank/aquariumBase.obj'

    var TANK_BASE_DIMENSIONS = {
        x: 0.8599,
        y: 1.8450,
        z: 2.1936
    };

    var BASE_VERTICAL_OFFSET = 0.47;

    var BUBBLE_SYSTEM_FORWARD_OFFSET = TANK_DIMENSIONS.x + 0.06;
    var BUBBLE_SYSTEM_LATERAL_OFFSET = 0.025;
    var BUBBLE_SYSTEM_VERTICAL_OFFSET = -0.30;

    var BUBBLE_SYSTEM_DIMENSIONS = {
        x: TANK_DIMENSIONS.x / 8,
        y: TANK_DIMENSIONS.y,
        z: TANK_DIMENSIONS.z / 8
    }

    var BUBBLE_SOUND_URL = "atp:/fishTank/aquarium_small.L.wav";
    var bubbleSound = SoundCache.getSound(BUBBLE_SOUND_URL);


    var ANEMONE_FORWARD_OFFSET = -TANK_DIMENSIONS.x+0.06;
    var ANEMONE_LATERAL_OFFSET = 0.2;
    var ANEMONE_VERTICAL_OFFSET = -0.16;

    var ANEMONE_MODEL_URL = 'atp:/fishTank/anemone.fbx';
    var ANEMONE_ANIMATION_URL = 'atp:/fishTank/anemone.fbx';
    var ANEMONE_DIMENSIONS = {
        x: 0.4,
        y: 0.4,
        z: 0.4
    }

    var ROCKS_FORWARD_OFFSET = 0;
    var ROCKS_LATERAL_OFFSET = 0.0;
    var ROCKS_VERTICAL_OFFSET = (-TANK_DIMENSIONS.y / 2) + 0.25;

    var ROCK_MODEL_URL = 'atp:/fishTank/Aquarium-Rocks-2.fbx';

    var ROCK_DIMENSIONS = {
        x: 0.707,
        y: 0.33,
        z: 1.64
    }

    var TREASURE_FORWARD_OFFSET = TANK_DIMENSIONS.x - 0.075;
    var TREASURE_LATERAL_OFFSET = 0.15;
    var TREASURE_VERTICAL_OFFSET = -0.26;

    var TREASURE_MODEL_URL = 'atp:/fishTank/Treasure-Chest2-SM.fbx';

    var TREASURE_DIMENSIONS = {
        x: 0.1199,
        y: 0.1105,
        z: 0.1020
    }

    function createFishTank() {
        var tankProperties = {
            name: 'hifi-home-fishtank',
            type: 'Model',
            modelURL: TANK_MODEL_URL,
            dimensions: TANK_DIMENSIONS,
            position: TANK_POSITION,
            rotation: Quat.fromPitchYawRollDegrees(spawnRotation.x, spawnRotation.y, spawnRotation.z),
            color: DEBUG_COLOR,
            collisionless: true,
            visible: true,
            userData: JSON.stringify({
                'hifiHomeKey': {
                    'reset': true
                }
            }),
        }

        fishTank = Entities.addEntity(tankProperties);
    }

    function createBubbleSystems() {

        var tankProperties = Entities.getEntityProperties(fishTank);
        var bubbleProperties = {
            "name": 'hifi-home-fishtank-bubbles',
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
                "y": 0.3,
                "z": 0
            },
            "accelerationSpread": {
                "x": 0.01,
                "y": 0.01,
                "z": 0.01
            },
            "particleRadius": 0.005,
            "radiusSpread": 0,
            "radiusStart": 0.01,
            "radiusFinish": 0.01,
            "alpha": 0.2,
            "alphaSpread": 0,
            "alphaStart": 0.3,
            "alphaFinish": 0,
            "emitterShouldTrail": 0,
            "textures": "atp:/fishTank/bubble-white.png",
            userData: JSON.stringify({
                'hifiHomeKey': {
                    'reset': true
                }
            }),
        };

        bubbleProperties.type = "ParticleEffect";
        bubbleProperties.parentID = fishTank;
        bubbleProperties.dimensions = BUBBLE_SYSTEM_DIMENSIONS;

        var finalOffset = getOffsetFromTankCenter(BUBBLE_SYSTEM_VERTICAL_OFFSET, BUBBLE_SYSTEM_FORWARD_OFFSET, BUBBLE_SYSTEM_LATERAL_OFFSET);

        bubbleProperties.position = finalOffset;
        bubbleSystem = Entities.addEntity(bubbleProperties);

        bubbleProperties.position.x += -0.076;
        secondBubbleSystem = Entities.addEntity(bubbleProperties)

        bubbleProperties.position.x += -0.076;
        thirdBubbleSystem = Entities.addEntity(bubbleProperties)

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

    function createRocks() {
        var finalPosition = getOffsetFromTankCenter(ROCKS_VERTICAL_OFFSET, ROCKS_FORWARD_OFFSET, ROCKS_LATERAL_OFFSET);

        var properties = {
            name: 'hifi-home-fishtank-rock',
            type: 'Model',
            parentID: fishTank,
            modelURL: ROCK_MODEL_URL,
            position: finalPosition,
            dimensions: ROCK_DIMENSIONS,
            rotation: Quat.fromPitchYawRollDegrees(0, 180, 0),
            userData: JSON.stringify({
                'hifiHomeKey': {
                    'reset': true
                }
            }),
        }

        rocks = Entities.addEntity(properties);
    }

    function createAnenome() {
        var finalPosition = getOffsetFromTankCenter(ANEMONE_VERTICAL_OFFSET, ANEMONE_FORWARD_OFFSET, ANEMONE_LATERAL_OFFSET);

        var properties = {
            name: 'hifi-home-fishtank-anemone',
            type: 'Model',
            animationURL: ANEMONE_ANIMATION_URL,
            animationIsPlaying: true,
            animationFPS: 15,
            animationSettings: JSON.stringify({
                hold: false,
                loop: true,
                running: true,
                startAutomatically: true
            }),
            parentID: fishTank,
            modelURL: ANEMONE_MODEL_URL,
            position: finalPosition,
            shapeType: 'Sphere',
            rotation: Quat.fromPitchYawRollDegrees(0, 270, 0),
            dimensions: ANEMONE_DIMENSIONS,
            userData: JSON.stringify({
                'hifiHomeKey': {
                    'reset': true
                }
            }),
        }

        anemone = Entities.addEntity(properties);

    }

    function createTreasureChest() {
        var finalPosition = getOffsetFromTankCenter(TREASURE_VERTICAL_OFFSET, TREASURE_FORWARD_OFFSET, TREASURE_LATERAL_OFFSET);

        var properties = {
            name: 'hifi-home-fishtank-treasure-chest',
            type: 'Model',
            parentID: fishTank,
            modelURL: TREASURE_MODEL_URL,
            position: finalPosition,
            dimensions: TREASURE_DIMENSIONS,
            rotation: Quat.fromPitchYawRollDegrees(10, 35, 10),
            userData: JSON.stringify({
                'hifiHomeKey': {
                    'reset': true
                }
            }),
        }

        treasure = Entities.addEntity(properties);
    }

    function createTankBase() {
        var properties = {
            name: 'hifi-home-fishtank-base',
            type: 'Model',
            modelURL: TANK_BASE_MODEL_URL,
            parentID: fishTank,
            shapeType: 'compound',
            compoundShapeURL: TANK_BASE_COLLISION_HULL,
            position: {
                x: TANK_POSITION.x,
                y: TANK_POSITION.y - BASE_VERTICAL_OFFSET,
                z: TANK_POSITION.z
            },
            rotation: Quat.fromPitchYawRollDegrees(0, 180, 0),
            dimensions: TANK_BASE_DIMENSIONS,
            userData: JSON.stringify({
                'hifiHomeKey': {
                    'reset': true
                }
            }),
        }

        tankBase = Entities.addEntity(properties);
    }

    function createInnerTank() {
        var properties = {
            dimensions: INNER_TANK_DIMENSIONS,
            name: 'hifi-home-fishtank-inner-tank',
            type: 'Box',
            visible: true,
            color: {
                red: 255,
                green: 0,
                blue: 255
            },
            position: TANK_POSITION,
            rotation: Quat.fromPitchYawRollDegrees(spawnRotation.x, spawnRotation.y, spawnRotation.z),
            collisionless: true,
            script: TANK_SCRIPT,
            visible: false,
            userData: JSON.stringify({
                'hifiHomeKey': {
                    'reset': true
                }
            }),
        }
        print('INNER TANK PROPS ' + properties)
        innerTank = Entities.addEntity(properties);
    }

    createFishTank();

    createBubbleSystems();

    createInnerTank();

    createAnenome();

    createRocks();

    createTankBase();

    createTreasureChest();

    function cleanup() {
        Entities.deleteEntity(fishTank);
        Entities.deleteEntity(tankBase);
        Entities.deleteEntity(bubbleSystem);
        Entities.deleteEntity(secondBubbleSystem);
        Entities.deleteEntity(thirdBubbleSystem);
        Entities.deleteEntity(anemone);
        Entities.deleteEntity(rocks);
        Entities.deleteEntity(innerTank);
    }

    this.cleanup = cleanup;
}