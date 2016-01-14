Script.include("../libraries/utils.js");

var shootingRangeURL = "https://s3.amazonaws.com/hifi-public/eric/models/shootingRange/shootingRange.fbx";
var floorURL = "https://s3.amazonaws.com/hifi-public/eric/models/shootingRange/shootingRange.fbx";
MyAvatar.bodyYaw = 0;
var rangePosition = Vec3.sum(MyAvatar.position, {
    x: 0,
    y: 0,
    z: -30
});
var rangeDimensions = {
    x: 44,
    y: 29,
    z: 96
}
var shootingRange = Entities.addEntity({
    type: 'Model',
    modelURL: shootingRangeURL,
    position: rangePosition,
    dimensions: rangeDimensions
});


var floorPosition = Vec3.subtract(rangePosition, {
    x: 0,
    y: 2,
    z: 0
});
var shootingRangeFloor = Entities.addEntity({
    type: "Model",
    modelURL: floorURL,
    shapeType: 'box',
    position: floorPosition,
    dimensions: {
        x: 93,
        y: 1,
        z: 93
    }
})

var monsters = [];
var numMonsters = 10;
var monsterURLS = ["https://s3.amazonaws.com/hifi-public/eric/models/shootingRange/monster1.fbx", "https://s3.amazonaws.com/hifi-public/eric/models/shootingRange/monster2.fbx"]
initMonsters();

function initMonsters() {
    for (var i = 0; i < numMonsters; i++) {

        var index = randInt(0, monsterURLS.length);
        var monsterURL = monsterURLS[index]
        var monsterPosition = Vec3.sum(rangePosition, {
            x: -rangeDimensions.x / 2 - i * randFloat(5, 10),
            y: 0,
            z: randFloat(-10, 10)
        });
        var monster = Entities.addEntity({
            type: "Model",
            modelURL: monsterURL,
            position: monsterPosition,
            dimensions: {
                x: 1.5,
                y: 1.6,
                z: 0.07
            },
            dynamic: true,
            shapeType: 'box',
            velocity: {
                x: randFloat(1, 3),
                y: 0,
                z: 0
            },
            damping: 0
        });

        monsters.push(monster);
    }
}

function checkMonsters() {

    // check monsters to see if they've gone out of bounds, if so, set them back to starting point
    monsters.forEach(function(monster) {
        var position = Entities.getEntityProperties(monster, "position").position;
        if (position.x > rangePosition.x + rangeDimensions.x / 2 ||
            position.z < rangePosition.z - rangeDimensions.z / 2 ||
            position.y < rangePosition.y - rangeDimensions.y / 2 || position.y > rangePosition.y + rangeDimensions.y / 2) {
            var monsterPosition = Vec3.sum(rangePosition, {
                x: -rangeDimensions.x / 2 - randFloat(5, 10),
                y: 0,
                z: randFloat(-10, 10)
            });
            Entities.editEntity(monster, {
                position: monsterPosition,
                velocity: {
                    x: randFloat(1, 3),
                    y: 0,
                    z: 0
                },
                angularVelocity: {x: 0, y: 0, z:0},
                rotation: Quat.fromPitchYawRollDegrees(0, 0, 0),
            });
        }
    });
}

var checkMonsterInterval = Script.setInterval(checkMonsters, 1000);

function cleanup() {
    Script.clearInterval(checkMonsterInterval);
    Entities.deleteEntity(shootingRange);
    Entities.deleteEntity(shootingRangeFloor);
    monsters.forEach(function(monster) {
        Entities.deleteEntity(monster);
    });
}

Script.scriptEnding.connect(cleanup);
