Script.include('pistol/pistolSpawner.js');

var shootingRangeURL = "file:///C:/Users/Eric/Desktop/shootingRange/shootingRange.fbx?v1"
var floorURL = "file:///C:/Users/Eric/Desktop/shootingRange/shootingRangeFloor.fbx?v1"

var rangePosition = Vec3.sum(MyAvatar.position, {x: 0, y: 0, z: -30})
var shootingRange = Entities.addEntity({
    type: 'Model',
    modelURL: shootingRangeURL,
    position: rangePosition,
    dimensions: {x: 44, y: 29, z: 96}
});


var floorPosition = Vec3.subtract(rangePosition, {x: 0, y: 2, z: 0});
var shootingRangeFloor = Entities.addEntity({
    type: "Model",
    modelURL: floorURL,
    shapeType: 'box',
    position: floorPosition,
    dimensions: {x: 93, y: 1, z: 93}
})

var monsterPosition = Vec3.sum(MyAvatar.position, Vec3.multiply(3, Quat.getFront(Camera.getOrientation())));
var monsterURL = "file:///C:/Users/Eric/Desktop/shootingRange/monster2.fbx?v7"
var monster = Entities.addEntity({
    type: "Model",
    modelURL: monsterURL,
    position: monsterPosition,
    dimensions: {x: 1.5, y: 1.6, z: 0.07},
    collisionsWillMove: true,
    shapeType: 'box'
});

function cleanup() {
    Entities.deleteEntity(shootingRange);
    Entities.deleteEntity(shootingRangeFloor);
    Entities.deleteEntity(monster);
}

Script.scriptEnding.connect(cleanup);