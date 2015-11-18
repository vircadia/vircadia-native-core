var center = Vec3.sum(MyAvatar.position, Vec3.multiply(3, Quat.getFront(Camera.getOrientation())));
var scriptURL = Script.resolvePath('pistol.js');
var modelURL = "https://s3.amazonaws.com/hifi-public/eric/models/gun.fbx";

var pistol = Entities.addEntity({
    type: 'Model',
    modelURL: modelURL,
    position: center,
    dimensions: {x: 0.08, y: .38, z: .6},
    script: scriptURL,
    color: {red: 200, green: 0, blue: 20},
    shapeType: 'box',
    collisionsWillMove: true,
    userData: JSON.stringify({
        grabbableKey: {
            spatialKey: {
                relativePosition: {x: 0, y: 0, z: 0},
                relativeRotation: {x: 0, y: 0, z: 0, w: 1}
            }
        }
    })
});


function cleanup() {
    Entities.deleteEntity(pistol);
}

Script.scriptEnding.connect(cleanup);