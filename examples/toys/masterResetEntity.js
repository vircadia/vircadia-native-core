//
//  Created by Eric Levin on 9/23/2015
//  Copyright 2015 High Fidelity, Inc.
//
//  Distributed under the Apache License, Version 2.0.
//  See the accompanying file LICENSE or http://www.apache.org/licenses/LICENSE-2.0.html
//

/*global print, MyAvatar, Entities, AnimationCache, SoundCache, Scene, Camera, Overlays, Audio, HMD, AvatarList, AvatarManager, Controller, UndoStack, Window, Account, GlobalServices, Script, ScriptDiscoveryService, LODManager, Menu, Vec3, Quat, AudioDevice, Paths, Clipboard, Settings, XMLHttpRequest, randFloat, randInt, pointInExtents, vec3equal, setEntityCustomData, getEntityCustomData */
//per script
/*global deleteAllToys, createAllToys, createGates, createBasketBall, createSprayCan, createDoll, createWand, createDice, createCat, deleteAllToys, createFlashlight, createBlocks, createMagballs, createLightSwitches */
var utilitiesScript = Script.resolvePath("../libraries/utils.js");
Script.include(utilitiesScript);

var resetKey = "resetMe";
var GRABBABLE_DATA_KEY = "grabbableKey";

var HIFI_PUBLIC_BUCKET = "http://s3.amazonaws.com/hifi-public/";

var shouldDeleteOnEndScript = false;

//Before creating anything, first search a radius and delete all the things that should be deleted
deleteAllToys();
createAllToys();

function createAllToys() {
    createBlocks({
        x: 548.3,
        y: 495.55,
        z: 504.4
    });

    createSprayCan({
        x: 549.7,
        y: 495.6,
        z: 503.91
    });

    createBasketBall({
        x: 547.73,
        y: 495.5,
        z: 505.47
    });

    createDoll({
        x: 546.67,
        y: 495.41,
        z: 505.09
    });

    createWand({
        x: 546.71,
        y: 495.55,
        z: 506.15
    });

    createDice();

    createFlashlight({
        x: 545.72,
        y: 495.41,
        z: 505.78
    });

    createCat({
        x: 551.09,
        y: 494.98,
        z: 503.49
    });

    // //Handles toggling of all sconce lights 
    createLightSwitches();



    createCombinedArmChair({
        x: 549.29,
        y: 495.05,
        z: 508.22
    })

    createPottedPlant({
        x: 554.26,
        y: 495.23,
        z: 504.53
    });


    createGates();

    createFire();
}

function deleteAllToys() {
    var entities = Entities.findEntities(MyAvatar.position, 100);

    entities.forEach(function(entity) {
        //params: customKey, id, defaultValue
        var shouldReset = getEntityCustomData(resetKey, entity, {}).resetMe;
        if (shouldReset === true) {
            Entities.deleteEntity(entity);
        }
    });
}

function createFire() {


    myOrientation = Quat.fromPitchYawRollDegrees(-90, 0, 0.0);

    var animationSettings = JSON.stringify({
        fps: 30,
        running: true,
        loop: true,
        firstFrame: 1,
        lastFrame: 10000
    });


    var fire = Entities.addEntity({
        type: "ParticleEffect",
        name: "fire",
        animationSettings: animationSettings,
        textures: "https://hifi-public.s3.amazonaws.com/alan/Particles/Particle-Sprite-Smoke-1.png",
        position: {
            x: 551.45,
            y: 494.82,
            z: 502.05
        },
        emitRate: 100,
        colorStart: {
            red: 70,
            green: 70,
            blue: 137
        },
        color: {
            red: 200,
            green: 99,
            blue: 42
        },
        colorFinish: {
            red: 255,
            green: 99,
            blue: 32
        },
        radiusSpread: .01,
        radiusStart: .02,
        radiusEnd: 0.001,
        particleRadius: .05,
        radiusFinish: 0.0,
        emitOrientation: myOrientation,
        emitSpeed: .3,
        speedSpread: 0.1,
        alphaStart: 0.05,
        alpha: 0.1,
        alphaFinish: 0.05,
        emitDimensions: {
            x: 1,
            y: 1,
            z: .1
        },
        polarFinish: 0.1,
        emitAcceleration: {
            x: 0.0,
            y: 0.0,
            z: 0.0
        },
        accelerationSpread: {
            x: 0.1,
            y: 0.01,
            z: 0.1
        },
        lifespan: 1
    });


    setEntityCustomData(resetKey, fire, {
        resetMe: true
    });
}


function createCat(position) {
    var scriptURL = Script.resolvePath("cat.js?v1");
    var modelURL = "http://hifi-public.s3.amazonaws.com/ryan/Dark_Cat.fbx";
    var animationURL = "http://hifi-public.s3.amazonaws.com/ryan/sleeping.fbx";
    var animationSettings = JSON.stringify({
        running: true,
    });
    var cat = Entities.addEntity({
        type: "Model",
        modelURL: modelURL,
        name: "cat",
        script: scriptURL,
        animationURL: animationURL,
        animationSettings: animationSettings,
        position: position,
        rotation: {
            w: 0.35020983219146729,
            x: -4.57763671875e-05,
            y: 0.93664455413818359,
            z: -1.52587890625e-05
        },
        dimensions: {
            x: 0.15723302960395813,
            y: 0.50762706995010376,
            z: 0.90716040134429932
        },
    });

    setEntityCustomData(resetKey, cat, {
        resetMe: true
    });
}

function createFlashlight(position) {
    var scriptURL = Script.resolvePath('flashlight/flashlight.js');
    var modelURL = "https://hifi-public.s3.amazonaws.com/models/props/flashlight.fbx";

    var flashlight = Entities.addEntity({
        type: "Model",
        modelURL: modelURL,
        name: "flashlight",
        script: scriptURL,
        position: position,
        dimensions: {
            x: 0.08,
            y: 0.30,
            z: 0.08
        },
        collisionsWillMove: true,
        gravity: {
            x: 0,
            y: -3.5,
            z: 0
        },
        velocity: {
            x: 0,
            y: -0.01,
            z: 0
        },
        shapeType: 'box',
    });

    setEntityCustomData(resetKey, flashlight, {
        resetMe: true
    });

}

function createLightSwitches() {
    var modelURL = "http://hifi-public.s3.amazonaws.com/ryan/lightswitch.fbx?v1";
    var scriptURL = Script.resolvePath("lightSwitchHall.js?v1");

    var lightSwitchHall = Entities.addEntity({
        type: "Model",
        modelURL: modelURL,
        name: "Light Switch Hall",
        script: scriptURL,
        position: {
            x: 543.27764892578125,
            y: 495.67999267578125,
            z: 511.00564575195312
        },
        rotation: {
            w: 0.63280689716339111,
            x: 0.63280689716339111,
            y: -0.31551080942153931,
            z: 0.31548023223876953
        },
        dimensions: {
            x: 0.10546875,
            y: 0.032372996211051941,
            z: 0.16242524981498718
        }
    });

    setEntityCustomData(resetKey, lightSwitchHall, {
        resetMe: true
    });

    scriptURL = Script.resolvePath("lightSwitchGarage.js?v1");

    var lightSwitchGarage = Entities.addEntity({
        type: "Model",
        modelURL: modelURL,
        name: "Light Switch Garage",
        script: scriptURL,
        position: {
            x: 545.62,
            y: 495.68,
            z: 500.21
        },
        rotation: {
            w: 0.20082402229309082,
            x: 0.20082402229309082,
            y: -0.67800414562225342,
            z: 0.67797362804412842
        },
        dimensions: {
            x: 0.10546875,
            y: 0.032372996211051941,
            z: 0.16242524981498718
        }
    });

    setEntityCustomData(resetKey, lightSwitchGarage, {
        resetMe: true
    });

}

function createDice() {
    var diceProps = {
        type: "Model",
        modelURL: "http://s3.amazonaws.com/hifi-public/models/props/Dice/goldDie.fbx",
        collisionSoundURL: "http://s3.amazonaws.com/hifi-public/sounds/dice/diceCollide.wav",
        name: "dice",
        position: {
            x: 541,
            y: 494.96,
            z: 509.1
        },
        dimensions: {
            x: 0.09,
            y: 0.09,
            z: 0.09
        },
        gravity: {
            x: 0,
            y: -3.5,
            z: 0
        },
        velocity: {
            x: 0,
            y: -0.01,
            z: 0
        },
        shapeType: "box",
        collisionsWillMove: true
    };
    var dice1 = Entities.addEntity(diceProps);

    diceProps.position = {
        x: 541.05,
        y: 494.96,
        z: 509.0
    };

    var dice2 = Entities.addEntity(diceProps);

    setEntityCustomData(resetKey, dice1, {
        resetMe: true
    });

    setEntityCustomData(resetKey, dice2, {
        resetMe: true
    });
}


function createGates() {
    var MODEL_URL = 'http://hifi-public.s3.amazonaws.com/ryan/fence.fbx';

    var rotation1 = Quat.fromPitchYawRollDegrees(0, 36, 0);

    var gate1 = Entities.addEntity({
        name: 'Back Door Gate',
        type: 'Model',
        shapeType: 'box',
        modelURL: MODEL_URL,
        position: {
            x: 546.52,
            y: 494.76,
            z: 498.87
        },
        dimensions: {
            x: 1.42,
            y: 1.13,
            z: 0.25
        },
        rotation: rotation1,
        collisionsWillMove: true,
        gravity: {
            x: 0,
            y: -100,
            z: 0
        },
        linearDamping: 1,
        angularDamping: 10,
        mass: 10,

    });

    setEntityCustomData(resetKey, gate1, {
        resetMe: true
    });

    setEntityCustomData(GRABBABLE_DATA_KEY, gate1, {
        grabbable: false
    });

    var rotation2 = Quat.fromPitchYawRollDegrees(0, -16, 0);
    var gate2 = Entities.addEntity({
        name: 'Front Door Fence',
        type: 'Model',
        modelURL: MODEL_URL,
        shapeType: 'box',
        position: {
            x: 531.15,
            y: 495.11,
            z: 520.20
        },
        dimensions: {
            x: 1.42,
            y: 1.13,
            z: 0.2
        },
        rotation: rotation2,
        collisionsWillMove: true,
        gravity: {
            x: 0,
            y: -100,
            z: 0
        },
        linearDamping: 1,
        angularDamping: 10,
        mass: 10,
    });

    setEntityCustomData(resetKey, gate2, {
        resetMe: true
    });

    setEntityCustomData(GRABBABLE_DATA_KEY, gate2, {
        grabbable: false
    });
}

function createWand(position) {
    var WAND_MODEL = 'http://hifi-public.s3.amazonaws.com/james/bubblewand/models/wand/wand.fbx';
    var WAND_COLLISION_SHAPE = 'http://hifi-public.s3.amazonaws.com/james/bubblewand/models/wand/actual_no_top_collision_hull.obj';
    var scriptURL = Script.resolvePath("bubblewand/wand.js");

    var entity = Entities.addEntity({
        name: 'Bubble Wand',
        type: "Model",
        modelURL: WAND_MODEL,
        position: position,
        gravity: {
            x: 0,
            y: -9.8,
            z: 0
        },
        dimensions: {
            x: 0.05,
            y: 0.25,
            z: 0.05
        },
        //must be enabled to be grabbable in the physics engine
        shapeType: 'compound',
        collisionsWillMove: true,
        compoundShapeURL: WAND_COLLISION_SHAPE,
        //Look into why bubble wand is going through table when gravity is enabled
        // gravity: {x: 0, y: -3.5, z: 0},
        // velocity: {x: 0, y: -0.01, z:0},
        script: scriptURL
    });

    setEntityCustomData(resetKey, entity, {
        resetMe: true
    });
}

function createBasketBall(position) {

    var modelURL = "http://s3.amazonaws.com/hifi-public/models/content/basketball2.fbx";

    var entity = Entities.addEntity({
        type: "Model",
        modelURL: modelURL,
        position: position,
        collisionsWillMove: true,
        shapeType: "sphere",
        name: "basketball",
        dimensions: {
            x: 0.25,
            y: 0.26,
            z: 0.25
        },
        gravity: {
            x: 0,
            y: -7,
            z: 0
        },
        restitution: 10,
        linearDamping: 0.0,
        velocity: {
            x: 0,
            y: -0.01,
            z: 0
        },
        collisionSoundURL: "http://s3.amazonaws.com/hifi-public/sounds/basketball/basketball.wav"
    });

    setEntityCustomData(resetKey, entity, {
        resetMe: true
    });
}

function createDoll(position) {
    var modelURL = "http://hifi-public.s3.amazonaws.com/models/Bboys/bboy2/bboy2.fbx";
    var scriptURL = Script.resolvePath("doll/doll.js?v2");

    var naturalDimensions = {
        x: 1.63,
        y: 1.67,
        z: 0.26
    };
    var desiredDimensions = Vec3.multiply(naturalDimensions, 0.15);
    var entity = Entities.addEntity({
        type: "Model",
        name: "doll",
        modelURL: modelURL,
        script: scriptURL,
        position: position,
        shapeType: 'box',
        dimensions: desiredDimensions,
        gravity: {
            x: 0,
            y: -5,
            z: 0
        },
        velocity: {
            x: 0,
            y: -0.1,
            z: 0
        },
        collisionsWillMove: true
    });

    setEntityCustomData(resetKey, entity, {
        resetMe: true
    });
}

function createSprayCan(position) {
    var scriptURL = Script.resolvePath("sprayPaintCan.js?v1" + Math.random());
    var modelURL = "https://hifi-public.s3.amazonaws.com/eric/models/paintcan.fbx";

    var entity = Entities.addEntity({
        type: "Model",
        name: "spraycan",
        modelURL: modelURL,
        position: position,
        dimensions: {
            x: 0.07,
            y: 0.17,
            z: 0.07
        },
        collisionsWillMove: true,
        shapeType: 'box',
        script: scriptURL,
        gravity: {
            x: 0,
            y: -0.5,
            z: 0
        },
        velocity: {
            x: 0,
            y: -1,
            z: 0
        }
    });

    setEntityCustomData(resetKey, entity, {
        resetMe: true
    });

}

//createPottedPlant,createArmChair,createPillow
function createPottedPlant(position) {
    var modelURL = "http://hifi-public.s3.amazonaws.com/models/potted_plant/potted_plant.fbx";

    var rotation = Quat.fromPitchYawRollDegrees(0, 0, 0);

    var entity = Entities.addEntity({
        type: "Model",
        name: "Potted Plant",
        modelURL: modelURL,
        position: position,
        dimensions: {
            x: 1.10,
            y: 2.18,
            z: 1.07
        },
        collisionsWillMove: true,
        shapeType: 'box',
        gravity: {
            x: 0,
            y: -9.8,
            z: 0
        },
        velocity: {
            x: 0,
            y: 0,
            z: 0
        },
        linearDamping: 0.4
    });

    setEntityCustomData(resetKey, entity, {
        resetMe: true
    });


    setEntityCustomData(GRABBABLE_DATA_KEY, entity, {
        grabbable: false
    });
};


function createCombinedArmChair(position) {
    var modelURL = "http://hifi-public.s3.amazonaws.com/models/red_arm_chair/combined_chair.fbx";
    var RED_ARM_CHAIR_COLLISION_HULL = "http://hifi-public.s3.amazonaws.com/models/red_arm_chair/red_arm_chair_collision_hull.obj"

    var rotation = Quat.fromPitchYawRollDegrees(0, -143, 0);

    var entity = Entities.addEntity({
        type: "Model",
        name: "Red Arm Chair",
        modelURL: modelURL,
        shapeType: 'compound',
        compoundShapeURL: RED_ARM_CHAIR_COLLISION_HULL,
        position: position,
        rotation: rotation,
        dimensions: {
            x: 1.26,
            y: 1.56,
            z: 1.35
        },
        collisionsWillMove: true,
        gravity: {
            x: 0,
            y: -0.8,
            z: 0
        },
        velocity: {
            x: 0,
            y: 0,
            z: 0
        },
        linearDamping: 0.2
    });

    setEntityCustomData(resetKey, entity, {
        resetMe: true
    });

    setEntityCustomData(GRABBABLE_DATA_KEY, entity, {
        grabbable: false
    });
};

function createArmChair(position) {
    var modelURL = "http://hifi-public.s3.amazonaws.com/models/red_arm_chair/new_red_arm_chair.fbx";
    var RED_ARM_CHAIR_COLLISION_HULL = "http://hifi-public.s3.amazonaws.com/models/red_arm_chair/new_red_arm_chair_collision_hull.obj"

    var rotation = Quat.fromPitchYawRollDegrees(0, -143, 0);

    var entity = Entities.addEntity({
        type: "Model",
        name: "Red Arm Chair",
        modelURL: modelURL,
        shapeType: 'compound',
        compoundShapeURL: RED_ARM_CHAIR_COLLISION_HULL,
        position: position,
        rotation: rotation,
        dimensions: {
            x: 1.26,
            y: 1.56,
            z: 1.35
        },
        collisionsWillMove: true,
        gravity: {
            x: 0,
            y: -0.5,
            z: 0
        },
        velocity: {
            x: 0,
            y: 0,
            z: 0
        },
        linearDamping: 0.3
    });

    setEntityCustomData(resetKey, entity, {
        resetMe: true
    });
};

function createPillow(position) {
    var modelURL = "http://hifi-public.s3.amazonaws.com/models/red_arm_chair/red_arm_chair_pillow.fbx";
    var RED_ARM_CHAIR_PILLOW_COLLISION_HULL = "http://hifi-public.s3.amazonaws.com/models/red_arm_chair/red_arm_chair_pillow_collision_hull.obj"

    var rotation = Quat.fromPitchYawRollDegrees(-0.29, -143.05, 0.32);

    var entity = Entities.addEntity({
        type: "Model",
        name: "Red Arm Chair Pillow",
        modelURL: modelURL,
        shapeType: 'compound',
        compoundShapeURL: RED_ARM_CHAIR_PILLOW_COLLISION_HULL,
        position: position,
        rotation: rotation,
        dimensions: {
            x: 0.4,
            y: 0.4,
            z: 0.4
        },
        collisionsWillMove: true,
        ignoreForCollisions: false,
        gravity: {
            x: 0,
            y: -10.1,
            z: 0
        },
        restitution: 0,
        velocity: {
            x: 0,
            y: -0.1,
            z: 0
        },
        linearDamping: 1
    });

    setEntityCustomData(resetKey, entity, {
        resetMe: true
    });
};

function createBlocks(position) {
    var baseURL = HIFI_PUBLIC_BUCKET + "models/content/planky/";
    var collisionSoundURL = "https://hifi-public.s3.amazonaws.com/sounds/Collisions-otherorganic/ToyWoodBlock.L.wav";
    var NUM_BLOCKS_PER_COLOR = 4;
    var i, j;

    var blockTypes = [{
        url: "planky_blue.fbx",
        dimensions: {
            x: 0.05,
            y: 0.05,
            z: 0.25
        }
    }, {
        url: "planky_green.fbx",
        dimensions: {
            x: 0.1,
            y: 0.1,
            z: 0.25
        }
    }, {
        url: "planky_natural.fbx",
        dimensions: {
            x: 0.05,
            y: 0.05,
            z: 0.05
        }
    }, {
        url: "planky_yellow.fbx",
        dimensions: {
            x: 0.03,
            y: 0.05,
            z: 0.25
        }
    }, {
        url: "planky_red.fbx",
        dimensions: {
            x: 0.1,
            y: 0.05,
            z: 0.25
        }
    }, ];

    var modelURL, entity;
    for (i = 0; i < blockTypes.length; i++) {
        for (j = 0; j < NUM_BLOCKS_PER_COLOR; j++) {
            modelURL = baseURL + blockTypes[i].url;
            entity = Entities.addEntity({
                type: "Model",
                modelURL: modelURL,
                position: Vec3.sum(position, {
                    x: j / 10,
                    y: i / 10,
                    z: 0
                }),
                shapeType: 'box',
                name: "block",
                dimensions: blockTypes[i].dimensions,
                collisionsWillMove: true,
                collisionSoundURL: collisionSoundURL,
                gravity: {
                    x: 0,
                    y: -2.5,
                    z: 0
                },
                velocity: {
                    x: 0,
                    y: -0.01,
                    z: 0
                }
            });

            //customKey, id, data
            setEntityCustomData(resetKey, entity, {
                resetMe: true
            });
        }
    }
}

function cleanup() {
    deleteAllToys();
}

if (shouldDeleteOnEndScript) {

    Script.scriptEnding.connect(cleanup);
}

function randFloat(low, high) {
    return low + Math.random() * (high - low);
}

function randInt(low, high) {
    return Math.floor(randFloat(low, high));
}