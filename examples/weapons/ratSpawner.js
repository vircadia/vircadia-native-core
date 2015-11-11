var scriptURL = Script.resolvePath('rat.js');
var center = Vec3.sum(MyAvatar.position, Vec3.multiply(3, Quat.getFront(Camera.getOrientation())));
var rat = Entities.addEntity({
    type: "Box",
    name: 'rat',
    position: center,
    dimensions: {x: 0.4, y: 0.4, z: 0.4},
    color: {red: 200, green: 20, blue: 200},
    collisionsWillMove: true,
    script: scriptURL
});

function cleanup() {
    Entities.deleteEntity(rat);
}

Script.scriptEnding.connect(cleanup)