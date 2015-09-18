var utilitiesScript = Script.resolvePath("../libraries/utils.js");
Script.include(utilitiesScript);

var resetKey = "resetMe";

HIFI_PUBLIC_BUCKET = "http://s3.amazonaws.com/hifi-public/";


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

function createSprayCan(position) {
    var scriptURL = Script.resolvePath("../entityScripts/sprayPaintCan.js");
    var modelURL = "https://hifi-public.s3.amazonaws.com/eric/models/paintcan.fbx";

    var entity = Entities.addEntity({
        type: "Model",
        name: "spraycan",
        modelURL: modelURL,
        position: position ,
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
    var modelUrl = HIFI_PUBLIC_BUCKET + 'marketplace/hificontent/Games/blocks/block.fbx';
    var BASE_DIMENSIONS = Vec3.multiply({
        x: 0.2,
        y: 0.1,
        z: 0.8
    }, 0.2);
    var NUM_BLOCKS = 4;

    for (var i = 0; i < NUM_BLOCKS; i++) {
        var entity = Entities.addEntity({
            type: "Model",
            modelURL: modelUrl,
            position: Vec3.sum(position, {
                x: 0,
                y: i / 5,
                z: 0
            }),
            shapeType: 'box',
            name: "block",
            dimensions: Vec3.sum(BASE_DIMENSIONS, {
                x: Math.random() / 10,
                y: Math.random() / 10,
                z: Math.random() / 10
            }),
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

Script.scriptEnding.connect(cleanup);