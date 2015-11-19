var shootingRangeURL = "file:///C:/Users/Eric/Desktop/shootingRange.fbx?v1"
var floorURL = "file:///C:/Users/Eric/Desktop/shootingRangeFloor.fbx?v1"
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


function cleanup() {
    Entities.deleteEntity(shootingRange);
    Entities.deleteEntity(shootingRangeFloor);
}

Script.scriptEnding.connect(cleanup);