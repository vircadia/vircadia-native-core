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
        x: 549.12,
        y: 495.55,
        z: 503.77
    });

    createBasketBall({
        x: 548.1,
        y: 497,
        z: 504.6
    });

    createDoll({
        x: 545.9,
        y: 496,
        z: 506.2
    });

    createWand({
        x: 546.48,
        y: 495.63,
        z: 506.25
    });
}

function deleteAllToys() {
    var entities = Entities.findEntities(MyAvatar.position, 100);

    entities.forEach(function(entity) {
        //params: customKey, id, defaultValue
        var shouldReset = getEntityCustomData(resetKey, entity, false);
        if (shouldReset) {
            Entities.deleteEntity(entity);
        }
    })
}

function createWand(position) {
    var WAND_MODEL = 'http://hifi-public.s3.amazonaws.com/james/bubblewand/models/wand/wand.fbx';
    var WAND_COLLISION_SHAPE = 'http://hifi-public.s3.amazonaws.com/james/bubblewand/models/wand/collisionHull.obj';
    //Just using abs path for demo purposes on sunday, since this PR for wand has not been merged
    var scriptURL = "https://raw.githubusercontent.com/imgntn/hifi/ccf125c047426a2c481d3ee8c58a05fc6048fdde/examples/toys/bubblewand/wand.js"

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
    var animationURL = "https://hifi-public.s3.amazonaws.com/models/Bboys/zombie_scream.fbx";
    var animationSettings = JSON.stringify({
        running: false
    });

    var scriptURL = Script.resolvePath("entityScripts/doll.js");
    // var scriptURL = Script.resolvePath("../entityScripts/sprayPaintCan.js");

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
        animationSettings: animationSettings,
        animationURL: animationURL,
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
    var scriptURL = Script.resolvePath("../entityScripts/sprayPaintCan.js");
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
    var dimensionsArray = [{
        x: .1,
        y: 0.05,
        z: 0.25
    }, {
        x: 0.06,
        y: 0.04,
        z: 0.28
    }];
    var NUM_BLOCKS = 12;

    for (var i = 0; i < NUM_BLOCKS; i++) {
        var modelURL = baseURL + modelURLs[randInt(0, modelURLs.length)];
        var entity = Entities.addEntity({
            type: "Model",
            modelURL: modelURL,
            position: Vec3.sum(position, {
                x: 0,
                y: i / 10,
                z: 0
            }),
            shapeType: 'box',
            name: "block",
            dimensions: dimensionsArray[randInt(0, dimensionsArray.length)],
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