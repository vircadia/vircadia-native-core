var center = Vec3.sum(MyAvatar.position, Vec3.multiply(3, Quat.getFront(Camera.getOrientation())));
var scriptURL = Script.resolvePath("pistolScriptSpawner.js");
var modelURL = "http://s3.amazonaws.com/hifi-public/cozza13/gun/m1911-handgun+1.fbx";
var pistolSpawnerEntity = Entities.addEntity({
    type: 'Box',
    position: center,
    dimensions: {x: 0.38, y: 1.9, z: 3.02},
    script: scriptURL,
    visible: false,
    collisionless: true
});

var pistol = Entities.addEntity({
    type: 'Model',
    modelURL: modelURL,
    position: center,
    dimensions: {x: 0.38, y: 1.9, z: 3.02},
    script: scriptURL,
    color: {red: 200, green: 0, blue: 20},
    collisionless: true
});



function cleanup() {
    Entities.deleteEntity(pistolSpawnerEntity);
    Entities.deleteEntity(pistol);
}

// Script.update.connect(update);
Script.scriptEnding.connect(cleanup);
