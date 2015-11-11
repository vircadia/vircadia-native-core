var scriptURL = Script.resolvePath('rat.js');
var naturalDimensions = {x: 11.31, y: 3.18, z: 2.19};
var sizeScale = 0.1;
var dimensions = Vec3.multiply(naturalDimensions, sizeScale);
var center = Vec3.sum(MyAvatar.position, Vec3.multiply(3, Quat.getFront(Camera.getOrientation())));
var rat = Entities.addEntity({
    type: "Model",
    modelURL:  "https://hifi-public.s3.amazonaws.com/ryan/rat.fbx",
    name: 'rat',
    position: center,
    dimensions: dimensions,
    shapeType: 'box',
    color: {red: 200, green: 20, blue: 200},
    gravity: {x: 0, y: -9.8, z: 0},
    collisionsWillMove: true,
    script: scriptURL
});

function cleanup() {
    Entities.deleteEntity(rat);
}

Script.scriptEnding.connect(cleanup)