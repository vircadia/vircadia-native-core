var shootingRangeURL = "file:///C:/Users/Eric/Desktop/shootingRange/shootingRange.fbx?v1"
var floorURL = "file:///C:/Users/Eric/Desktop/shootingRange/shootingRangeFloor.fbx?v1"
var shootingRange = Entities.addEntity({
    type: 'Model',
    modelURL: shootingRangeURL,
    position: MyAvatar.position,
});


var floorPosition = Vec3.subtract(MyAvatar.position, {x: 0, y: 10, z: 0});
var shootingRangeFloor = Entities.addEntity({
    type: "Model",
    modelURL: floorURL,
    shapeType: 'box',
    position: floorPosition
})

var monsterPosition = Vec3.sum(MyAvatar.position, Vec3.multiply(3, Quat.getFront(Camera.getOrientation())));
var monsterURL = "file:///C:/Users/Eric/Desktop/shootingRange/monster1.fbx?v3"
var monster = Entities.addEntity({
    type: "Model",
    modelURL: monsterURL,
    position: monsterPosition
});

function cleanup() {
    Entities.deleteEntity(shootingRange);
    Entities.deleteEntity(shootingRangeFloor);
    Entities.deleteEntity(monster);
}

Script.scriptEnding.connect(cleanup);