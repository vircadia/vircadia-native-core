var center = Vec3.sum(MyAvatar.position, Vec3.multiply(1.5, Quat.getFront(Camera.getOrientation())));
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
    dynamic: true,
    gravity: {x: 0, y: -5.0, z: 0},
    restitution: 0,
    collisionSoundURL: "https://s3.amazonaws.com/hifi-public/sounds/Guns/Gun_Drop_and_Metalli_1.wav",
    userData: JSON.stringify({
        grabbableKey: {
            spatialKey: {
                relativePosition: {
                    x: 0,
                    y: 0.05,
                    z: -0.08
                },
                relativeRotation: Quat.fromPitchYawRollDegrees(90, 90, 0)
            },
            invertSolidWhileHeld: true
        }
    })
});


function cleanup() {
    Entities.deleteEntity(pistol);
}

Script.scriptEnding.connect(cleanup);
