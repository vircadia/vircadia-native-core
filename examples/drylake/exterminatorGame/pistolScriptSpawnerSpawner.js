var center = Vec3.sum(MyAvatar.position, Vec3.multiply(3, Quat.getFront(Camera.getOrientation())));
var scriptURL = Script.resolvePath("pistolScriptSpawner.js");

var pistolSpawnerEntity = Entities.addEntity({
    type: 'Box',
    position: center,
    dimensions: {x: 1, y: 1, z: 1},
    script: scriptURL,
    color: {red: 200, green: 0, blue: 20},
    ignoreForCollisions: true
});


function cleanup() {
    Entities.deleteEntity(pistolSpawnerEntity);
}

Script.scriptEnding.connect(cleanup);