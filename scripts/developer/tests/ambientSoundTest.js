var WAVE = 'http://cdn.rawgit.com/ambisonictoolkit/atk-sounds/aa31005c/stereo/Aurora_Surgit-Lux_Aeterna.wav';
var uuid = Entities.addEntity({
    type: "Shape",
    shape: "Icosahedron",
    dimensions: Vec3.HALF,
    script: Script.resolvePath('../../tutorials/entity_scripts/ambientSound.js'),
    position: Vec3.sum(Vec3.multiply(5, Quat.getForward(MyAvatar.orientation)), MyAvatar.position),
    userData: JSON.stringify({
        soundURL: WAVE,
        maxVolume: 0.1,
        range: 25,
        disabled: true,
        grabbableKey: { wantsTrigger: true },
    }),
    lifetime: 600,
});
Script.scriptEnding.connect(function() {
    Entities.deleteEntity(uuid);
});
