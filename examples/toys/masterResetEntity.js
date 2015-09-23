var utilitiesScript = Script.resolvePath("../libraries/utils.js");
Script.include(utilitiesScript);

var resetKey = "resetMe";

HIFI_PUBLIC_BUCKET = "http://s3.amazonaws.com/hifi-public/";

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
        x: 549.8,
        y: 495.6,
        z: 503.94
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
        x: 551.107421875,
        y: 494.60513305664062,
        z: 503.1910400390625
    });

    createMagballs({
        x: 548.73,
        y: 495.51,
        z: 503.54
    });

    //Handles toggling of all sconce lights 
    createLightSwitches();
}

function deleteAllToys() {
    var entities = Entities.findEntities(MyAvatar.position, 100);

    entities.forEach(function(entity) {
        //params: customKey, id, defaultValue
        var shouldReset = getEntityCustomData(resetKey, entity, {}).resetMe;
        if (shouldReset === true) {
            Entities.deleteEntity(entity);
        }
    })
}

function createMagballs(position) {


    var modelURL = "http://hifi-public.s3.amazonaws.com/ryan/tin2.fbx";
    var tinCan = Entities.addEntity({
        type: "Model",
        modelURL: modelURL,
        name: "Tin Can",
        position: position,
        rotation: {
            w: 0.93041884899139404,
            x: -1.52587890625e-05,
            y: 0.36647593975067139,
            z: -1.52587890625e-05
        },
        dimensions: {
            x: 0.16946873068809509,
            y: 0.21260403096675873,
            z: 0.16946862637996674
        },
    });


    setEntityCustomData(resetKey, tinCan, {
        resetMe: true
    });

    setEntityCustomData("OmniTool", tinCan, {
        script: "../toys/magBalls.js"
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
            w: 0.9510490894317627,
            x: -1.52587890625e-05,
            y: 0.30901050567626953,
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
            y: -.01,
            z: 0
        },
        shapeType: 'box',
    });

    setEntityCustomData(resetKey, flashlight, {
        resetMe: true
    });

}

function createLightSwitches() {
    var modelURL = "http://hifi-public.s3.amazonaws.com/ryan/dimmer.fbx";
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
            y: -.01,
            z: 0
        },
        shapeType: "box",
        collisionsWillMove: true
    }
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

function createWand(position) {
    var WAND_MODEL = 'http://hifi-public.s3.amazonaws.com/james/bubblewand/models/wand/wand.fbx';
    var WAND_COLLISION_SHAPE = 'http://hifi-public.s3.amazonaws.com/james/bubblewand/models/wand/collisionHull.obj';
    //Just using abs path for demo purposes on sunday, since this PR for wand has not been merged
    var scriptURL = Script.resolvePath("bubblewand/wand.js");

    var entity = Entities.addEntity({
        name: 'Bubble Wand',
        type: "Model",
        modelURL: WAND_MODEL,
        position: position,
        gravity: {
            x: 0,
            y: 0,
            z: 0,
        },
        dimensions: {
            x: 0.05,
            y: 0.25,
            z: 0.05
        },
        //must be enabled to be grabbable in the physics engine
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
            y: -.01,
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
    var scriptURL = Script.resolvePath("doll.js");

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
            y: -.1,
            z: 0
        },
        collisionsWillMove: true
    });

    setEntityCustomData(resetKey, entity, {
        resetMe: true
    });
}

function createSprayCan(position) {
    var scriptURL = Script.resolvePath("sprayPaintCan.js");
    var modelURL = "https://hifi-public.s3.amazonaws.com/eric/models/paintcan.fbx";

    var entity = Entities.addEntity({
        type: "Model",
        name: "spraycan",
        modelURL: modelURL,
        position: position,
        rotation: {
            x: 0,
            y: 0,
            z: 0,
            w: 1
        },
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

function createBlocks(position) {
    var baseURL = HIFI_PUBLIC_BUCKET + "models/content/planky/"
    var modelURLs = ['planky_blue.fbx', 'planky_green.fbx', 'planky_natural.fbx', "planky_red.fbx", "planky_yellow.fbx"];
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
        },


    ];
    var NUM_BLOCKS_PER_COLOR = 4;

    for (var i = 0; i < blockTypes.length; i++) {
        for (j = 0; j < NUM_BLOCKS_PER_COLOR; j++) {
            var modelURL = baseURL + blockTypes[i].url;
            var entity = Entities.addEntity({
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
                gravity: {
                    x: 0,
                    y: -2.5,
                    z: 0
                },
                velocity: {
                    x: 0,
                    y: -.01,
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