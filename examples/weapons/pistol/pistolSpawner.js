var center = Vec3.sum(MyAvatar.position, Vec3.multiply(0.5, Quat.getFront(Camera.getOrientation())));
var scriptURL = Script.resolvePath('pistol.js');
var modelURL = "https://s3.amazonaws.com/hifi-public/eric/models/gun.fbx";


var pistol = Entities.addEntity({
    type: 'Model',
    modelURL: modelURL,
    position: center,
    dimensions: {
        x: 0.05,
        y: .23,
        z: .36
    },
    script: scriptURL,
    color: {
        red: 200,
        green: 0,
        blue: 20
    },
    shapeType: 'box',
    collisionsWillMove: true,
    collisionSoundURL: "https://s3.amazonaws.com/hifi-public/sounds/Guns/Gun_Drop_and_Metalli_1.wav",
    userData: JSON.stringify({
        grabbableKey: {
            spatialKey: {
                relativePosition: {
                    x: 0,
                    y: 0,
                    z: 0
                },
                relativeRotation: Quat.fromPitchYawRollDegrees(45, 90, 0)
            },
            invertSolidWhileHeld: true
        }
    })
});


function cleanup() {
    Entities.deleteEntity(pistol);
}

// Script.scriptEnding.connect(cleanup);