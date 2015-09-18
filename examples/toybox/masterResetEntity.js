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

function createBlocks(position) {
    print("CREATE BLOCKS")
    var modelUrl = HIFI_PUBLIC_BUCKET + 'marketplace/hificontent/Games/blocks/block.fbx';
    var BASE_DIMENSIONS = Vec3.multiply({
        x: 0.2,
        y: 0.1,
        z: 0.8
    }, 0.2);
    var NUM_BLOCKS = 4;

    for (var i = 0; i < NUM_BLOCKS; i++) {
        var block = Entities.addEntity({
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
        setEntityCustomData(resetKey, block, {
            resetMe: true
        });
    }
}

function cleanup() {
    deleteAllToys();
}

Script.scriptEnding.connect(cleanup);