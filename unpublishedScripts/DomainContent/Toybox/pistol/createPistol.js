var center = Vec3.sum(MyAvatar.position, Vec3.multiply(1.5, Quat.getFront(Camera.getOrientation())));
var scriptURL = Script.resolvePath('pistol.js');
var modelURL = "http://hifi-production.s3.amazonaws.com/DomainContent/Toybox/pistol/gun.fbx";


var pistol = Entities.addEntity({
    type: 'Model',
    modelURL: modelURL,
    position: center,
    dimensions: {
        x: 0.05,
        y: 0.23,
        z: 0.36
    },
    script: scriptURL,
    color: {
        red: 200,
        green: 0,
        blue: 20
    },
    shapeType: 'box',
    dynamic: true,
    gravity: {
        x: 0,
        y: -5.0,
        z: 0
    },
    restitution: 0,
    damping:0.5,
    collisionSoundURL: "http://hifi-production.s3.amazonaws.com/DomainContent/Toybox/pistol/drop.wav",
    userData: JSON.stringify({
        grabbableKey: {
            invertSolidWhileHeld: true
        },
        wearable:{joints:{RightHand:[{x:0.07079616189002991,
                                      y:0.20177987217903137,
                                      z:0.06374628841876984},
                                     {x:-0.5863648653030396,
                                      y:-0.46007341146469116,
                                      z:0.46949487924575806,
                                      w:-0.4733745753765106}],
                          LeftHand:[{x:0.1802254319190979,
                                     y:0.13442856073379517,
                                     z:0.08504903316497803},
                                    {x:0.2198076844215393,
                                     y:-0.7377811074256897,
                                     z:0.2780133783817291,
                                     w:0.574519157409668}]}}
    })
});


function cleanup() {
    Entities.deleteEntity(pistol);
}

Script.scriptEnding.connect(cleanup);
