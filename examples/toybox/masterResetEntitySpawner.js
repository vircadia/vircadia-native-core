var scriptURL = Script.resolvePath("masterResetEntity.js?v1" + Math.random());

var center = Vec3.sum(MyAvatar.position, Vec3.multiply(1, Quat.getFront(Camera.getOrientation())));

var resetEntity = Entities.addEntity({
    type: "Box",
    dimensions: {x: .3, y: 0.3, z: 0.3},
    position: center,
    color: {red: 100, green: 10, blue: 100},
    script: scriptURL
});



function cleanup() {
    Entities.deleteEntity(resetEntity);
}

Script.scriptEnding.connect(cleanup);